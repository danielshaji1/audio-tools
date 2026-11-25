// SPDX-License-Identifier: GPL-3.0-or-later
#include "servicedesk.h"
#include <numeric>
#include "source/measurement.h"
#include <QtWidgets>
#include <QDesktopServices>
#include <QUrlQuery>
#include <cmath>

static QString rootFolder() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/Services";
}
ServiceDesk::ServiceDesk(SourceList *list) : sources(list) {
    setWindowTitle("Service timeline and room measurements");
    resize(540,500);
    auto *form=new QFormLayout(this);
    inputs=new QComboBox; form->addRow("Measurement",inputs);
    title=new QLineEdit("Sunday service"); form->addRow("Service name",title);
    minutes=new QSpinBox; minutes->setRange(1,180); minutes->setValue(15); form->addRow("Rolling Leq (minutes)",minutes);
    yellow=new QSpinBox; red=new QSpinBox;
    for(auto *box:{yellow,red}) box->setRange(0,160);
    QSettings prefs;
    yellow->setValue(prefs.value("service/yellow",85).toInt()); red->setValue(prefs.value("service/red",95).toInt());
    form->addRow("Yellow from (dBA)",yellow); form->addRow("Red from (dBA)",red);
    record=new QPushButton("Start service"); form->addRow(record);
    status=new QLabel("Calibrate the measurement input before recording SPL."); status->setWordWrap(true); form->addRow(status);
    connect(record,&QPushButton::clicked,this,[this]{ if(timeline.isOpen()) finish(); else start(); });
    auto *decay=new QPushButton("Estimate RT60 from selected impulse response"); form->addRow(decay);
    connect(decay,&QPushButton::clicked,this,&ServiceDesk::room);
    history=new QComboBox; form->addRow("Archived service",history);
    report=new QLabel;report->setWordWrap(true);report->setTextFormat(Qt::PlainText);form->addRow(report);
    connect(history,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{loadReport();});
    recipient=new QLineEdit(prefs.value("service/email").toString());form->addRow("Email to",recipient);
    auto *mail=new QPushButton("Compose report…"); form->addRow(mail);
    connect(mail,&QPushButton::clicked,this,[this]{
        if(summary.isEmpty()) return;
        QSettings().setValue("service/email",recipient->text());
        QUrl url;url.setScheme("mailto");url.setPath(recipient->text().trimmed());
        QUrlQuery query;query.addQueryItem("subject","Service measurement report");query.addQueryItem("body",summary);url.setQuery(query);
        if(!QDesktopServices::openUrl(url)) status->setText("Configure a default mail application to compose the report.");
    });
    auto *files=new QPushButton("Show service files in Finder");form->addRow(files);
    connect(files,&QPushButton::clicked,this,[this]{QDir().mkpath(rootFolder());QDesktopServices::openUrl(QUrl::fromLocalFile(rootFolder()+"/"+history->currentText()));});
    timer.setInterval(1000);timer.setTimerType(Qt::PreciseTimer);connect(&timer,&QTimer::timeout,this,&ServiceDesk::sample);
}
ServiceDesk::~ServiceDesk(){if(timeline.isOpen())finish();}
void ServiceDesk::present(){
    if(!timeline.isOpen()){
        auto previous=inputs->currentData(); inputs->clear();
        for(int i=0;i<sources->count();++i){auto s=sources->get(i);if(s && dynamic_cast<Measurement*>(s.get())) inputs->addItem(s->name(),s->uuid());}
        int index=inputs->findData(previous);if(index>=0)inputs->setCurrentIndex(index);
    }
    history->clear();history->addItems(QDir(rootFolder()).entryList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name|QDir::Reversed));loadReport();
    show();raise();activateWindow();
}
void ServiceDesk::start(){
    sourceId=inputs->currentData().toUuid();auto s=sources->getByUUid(sourceId);
    if(!s||!s->active()){status->setText("Select an active measurement input.");return;}
    if(red->value()<=yellow->value()){status->setText("Red must be above yellow.");return;}
    QSettings prefs;prefs.setValue("service/yellow",yellow->value());prefs.setValue("service/red",red->value());
    folder=rootFolder()+"/"+QDateTime::currentDateTime().toString("yyyy-MM-dd HH-mm-ss-zzz");
    activeTitle=title->text().trimmed(); if(activeTitle.isEmpty())activeTitle="Sunday service";
    timeline.setFileName(folder+"/timeline.jsonl");
    if(!QDir().mkpath(folder)||!timeline.open(QIODevice::WriteOnly|QIODevice::NewOnly)){status->setText("Could not create service timeline.");return;}
    total=windowEnergy=0;count=0;maximum=-200;powers.clear();
    for(QWidget *w:QList<QWidget*>{inputs,title,minutes,yellow,red})w->setEnabled(false);
    record->setText("Finish service");timer.start();status->setText("Recording "+activeTitle);
}
void ServiceDesk::sample(){
    auto s=sources->getByUUid(sourceId);
    if(!s||!s->active()){finish();status->setText("Recording ended because the input became unavailable.");return;}
    const double slow=s->level(Weighting::A,Meter::Slow)+140,fast=s->level(Weighting::A,Meter::Fast)+140;
    if(!std::isfinite(slow)||!std::isfinite(fast))return;
    double power=std::pow(10.,slow/10);powers.push_back(power);windowEnergy+=power;
    if(powers.size()>size_t(60*minutes->value())){windowEnergy-=powers.front();powers.pop_front();}
    const double rolling=10*std::log10(windowEnergy/powers.size());
    QJsonArray spectrum;
    s->lock();
    for(unsigned i=1;i<s->frequencyDomainSize();++i){
        const auto f=s->frequency(i),v=s->module(i);
        if(f>0 && v>0 && std::isfinite(v)) spectrum.append(QJsonArray{f,20*std::log10(v)});
    }s->unlock();
    QJsonObject row{{"service",activeTitle},{"time",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},{"source",s->name()},{"fast_dBA",fast},{"slow_dBA",slow},{"rolling_dBA",rolling},{"rolling_minutes",minutes->value()},{"rta_hz_dbfs",spectrum}};
    auto bytes=QJsonDocument(row).toJson(QJsonDocument::Compact)+'\n';
    if(timeline.write(bytes)!=bytes.size()||!timeline.flush()){finish();status->setText("Disk write failed. Earlier samples are in the service folder.");return;}
    ++count;total+=power;maximum=std::max(maximum,fast);
    QString state=slow>=red->value()?"Red":slow>=yellow->value()?"Yellow":"Green";
    status->setStyleSheet("color:"+QString(state=="Red"?"#ff7670":state=="Yellow"?"#ffd16d":"#98da80"));
    status->setText(QString("%1   ·   Fast %2   ·   Slow %3   ·   Leq %4 dBA   ·   %5 samples").arg(state).arg(fast,0,'f',1).arg(slow,0,'f',1).arg(rolling,0,'f',1).arg(count));
}
void ServiceDesk::finish(){
    timer.stop();timeline.close();
    QString body=activeTitle+"\n"+QFileInfo(folder).fileName()+" · "+inputs->currentText()+"\n";
    body+=count?QString("%1 one-second readings · Service Leq %2 dBA · Highest sampled fast level %3 dBA").arg(count).arg(10*std::log10(total/count),0,'f',1).arg(maximum,0,'f',1):"No valid readings.";
    QSaveFile out(folder+"/report.txt");auto data=body.toUtf8();
    bool saved=out.open(QIODevice::WriteOnly)&&out.write(data)==data.size()&&out.commit();
    for(QWidget *w:QList<QWidget*>{inputs,title,minutes,yellow,red})w->setEnabled(true);
    record->setText("Start service");status->setText(saved?"Service saved.":"Could not save the summary. The timeline is available in Finder.");
    history->insertItem(0,QFileInfo(folder).fileName());history->setCurrentIndex(0);loadReport();
}
void ServiceDesk::loadReport(){
    QFile file(rootFolder()+"/"+history->currentText()+"/report.txt");summary.clear();
    if(file.open(QIODevice::ReadOnly))summary=QString::fromUtf8(file.readAll());
    report->setText(summary);
}
void ServiceDesk::room(){
    auto s=sources->getByUUid(inputs->currentData().toUuid());if(!s){status->setText("Select a measurement with an impulse response.");return;}
    std::vector<double> p,t;
    s->lock();for(unsigned i=0;i<s->timeDomainSize();++i){double v=s->impulseValue(i);p.push_back(v*v);t.push_back(s->impulseTime(i)/1000.);}s->unlock();
    if(p.size()<100){status->setText("Capture a longer impulse response.");return;}
    size_t peak=std::max_element(p.begin(),p.end())-p.begin();
    auto tail=p.size()/10;double noise=std::accumulate(p.end()-tail,p.end(),0.)/tail;
    if(p[peak]<=std::max(1e-20,noise*3162)){status->setText("Insufficient impulse-to-noise ratio for T20.");return;}
    double sum=0;for(size_t i=p.size();i-->peak;){sum+=std::max(0.,p[i]-noise);p[i]=sum;}
    double n=0,x=0,y=0,xx=0,xy=0,yy=0;
    for(size_t i=peak;i<p.size();++i){double d=10*std::log10(p[i]/sum),time=t[i]-t[peak];if(d>=-25&&d<=-5){n++;x+=time;y+=d;xx+=time*time;yy+=d*d;xy+=time*d;}}
    double a=n*xy-x*y,b=n*xx-x*x,fit=a*a/(b*(n*yy-y*y));
    if(n<10||!(fit>0.95)||!(a<0&&b>0)){status->setText("No reliable −5 to −25 dB decay fit. Check routing and impulse length.");return;}
    status->setText(QString("Broadband RT60 estimate %1 s · T20 fit R² %2").arg(-60*b/a,0,'f',2).arg(fit,0,'f',3));
}
