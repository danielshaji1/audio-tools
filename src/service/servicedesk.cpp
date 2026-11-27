// SPDX-License-Identifier: GPL-3.0-or-later
#include "servicedesk.h"
#include "source/measurement.h"
#include <QtCore>
#include <QtGui>
#include <cmath>
#include <numeric>

static QString rootFolder() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/Services";
}
static bool saveBytes(const QString &path,const QByteArray &bytes) {
    QSaveFile f(path);
    return f.open(QIODevice::WriteOnly) && f.write(bytes)==bytes.size() && f.commit();
}
static bool deliverFolder(const QString &archive,const QString &destination){
    QFile job(archive+"/folder-pending");
    if(destination.isEmpty() || !QFileInfo(destination).isDir() || !job.open(QIODevice::ReadOnly))return false;
    auto name=QString::fromUtf8(job.readAll());
    if(name.isEmpty() || name!=QFileInfo(name).fileName())return false;
    QFile pdf(archive+"/report.pdf");
    if(!pdf.open(QIODevice::ReadOnly))return false;
    if(!saveBytes(QDir(destination).filePath(name),pdf.readAll()))return false;
    job.close();
    return QFile::remove(job.fileName());
}
static const char mailScript[]=R"OSA(on run argv
    set attachmentFile to POSIX file (item 3 of argv) as alias
    tell application "Mail"
        set msg to make new outgoing message with properties {subject:(item 2 of argv), content:"The attached service report includes the SPL timeline and measurement summary.", visible:false}
        tell msg
            make new to recipient at end of to recipients with properties {address:(item 1 of argv)}
            make new attachment with properties {file name:attachmentFile} at after last paragraph of content
        end tell
        if not (send msg) then error "Mail did not accept the report."
    end tell
end run)OSA";

ServiceDesk::ServiceDesk(SourceList *list):sources(list) {
    timer.setInterval(1000);timer.setTimerType(Qt::PreciseTimer);
    connect(&timer,&QTimer::timeout,this,&ServiceDesk::sample);
    connect(&mail,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus how){
        if(code==0 && how==QProcess::NormalExit){
            const bool saved=QFile::rename(dispatching+"/pending.json",dispatching+"/sent.json");
            notify(saved?"Report submitted to Apple Mail.":"Mail accepted the report. The archive delivery receipt could not be updated.");
        }else{
            notify("Report saved for retry. Check Mail account and macOS Automation permission. "+QString::fromUtf8(mail.readAllStandardError()).trimmed());
        }
        deliveryLock.reset(); dispatching.clear();
    });
    connect(&mail,&QProcess::errorOccurred,this,[this](QProcess::ProcessError e){
        if(e==QProcess::FailedToStart){
            deliveryLock.reset(); dispatching.clear();notify("Apple Mail automation could not start. The report remains queued.");
        }
    });
    retryTimer.setInterval(60000);
    connect(&retryTimer,&QTimer::timeout,this,&ServiceDesk::retry);
    if(!QCoreApplication::arguments().contains("--smoke-test"))retryTimer.start();
}
QStringList ServiceDesk::archives() const {
    return QDir(rootFolder()).entryList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name|QDir::Reversed);
}
void ServiceDesk::openArchive(QString archive){
    if(!archives().contains(archive))return;
    auto path=rootFolder()+"/"+archive;
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFile::exists(path+"/report.pdf")?path+"/report.pdf":path));
}
QVariantList ServiceDesk::levels() const {
    QVariantList list;
    int stride=std::max(1,slow.size()/600);
    for(int i=0;i<slow.size();i+=stride)list.append(slow[i]);
    if(!slow.empty() && (slow.size()-1)%stride)list.append(slow.last());
    return list;
}
void ServiceDesk::start(QString title,QUuid id){
    if(recording())return;
    auto s=sources->getByUUid(id);
    if(!s||!dynamic_cast<Measurement*>(s.get())||!s->active()){notify("Select an active measurement source.");return;}
    started=QDateTime::currentDateTime();
    folder=rootFolder()+"/"+started.toString("yyyy-MM-dd HH-mm-ss-zzz");
    sourceId=id;inputName=s->name();activeTitle=title.trimmed().left(160);
    if(activeTitle.isEmpty())activeTitle="Sunday service";
    timeline.setFileName(folder+"/timeline.jsonl");
    if(!QDir().mkpath(folder)||!timeline.open(QIODevice::WriteOnly|QIODevice::NewOnly)){notify("Could not create service timeline.");return;}
    slow.clear();fast.clear();elapsed.start();timer.start();
    notify("Recording "+activeTitle);sample();
}
void ServiceDesk::sample(){
    auto s=sources->getByUUid(sourceId);
    if(!s||!s->active()){finish();return;}
    double a=s->level(Weighting::A,Meter::Fast)+140,b=s->level(Weighting::A,Meter::Slow)+140;
    if(!std::isfinite(a)||!std::isfinite(b))return;
    QJsonArray spectrum;
    s->lock();
    for(unsigned i=1;i<s->frequencyDomainSize();++i){
        auto f=s->frequency(i),v=s->module(i);
        if(f>0&&v>0&&std::isfinite(v))spectrum.append(QJsonArray{f,20*std::log10(v)});
    }
    s->unlock();
    double sec=elapsed.elapsed()/1000.;
    QJsonObject row{{"service",activeTitle},{"source",inputName},{"time",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},{"elapsed_s",sec},{"fast_dBA",a},{"slow_dBA",b},{"rta_hz_dbfs",spectrum}};
    auto bytes=QJsonDocument(row).toJson(QJsonDocument::Compact)+'\n';
    if(timeline.write(bytes)!=bytes.size()||!timeline.flush()){finish();notify("Recording stopped after a disk write error. Check the service archive.");return;}
    slow.append({sec,b});fast.append({sec,a});emit changed();
}
bool ServiceDesk::writeReport(){
    QSaveFile file(folder+"/report.pdf");
    if(!file.open(QIODevice::WriteOnly))return false;
    {
        QPdfWriter pdf(&file);pdf.setResolution(96);pdf.setPageSize(QPageSize(QPageSize::A4));pdf.setTitle(activeTitle);
        QPainter p(&pdf);if(!p.isActive())return false;
        int width=pdf.width();
        p.setFont(QFont("Helvetica",20,QFont::Bold));p.drawText(QRect(12,12,width-24,80),Qt::TextWordWrap,activeTitle);
        p.setFont(QFont("Helvetica",11));
        p.drawText(12,112,started.toString("yyyy-MM-dd HH:mm:ss t")+"  –  "+QDateTime::currentDateTime().toString("HH:mm:ss t"));
        p.drawText(12,140,"Measurement: "+inputName);
        p.drawText(12,168,QString("Duration: %1 min     Samples: %2     Weighting: A").arg(elapsed.elapsed()/60000.,0,'f',1).arg(slow.size()));
        if(!slow.empty()){
            auto levelLess=[](QPointF a,QPointF b){return a.y()<b.y();};
            double lo=std::floor(std::min(std::min_element(slow.begin(),slow.end(),levelLess)->y(),std::min_element(fast.begin(),fast.end(),levelLess)->y())/10)*10;
            double hi=std::ceil(std::max(std::max_element(slow.begin(),slow.end(),levelLess)->y(),std::max_element(fast.begin(),fast.end(),levelLess)->y())/10)*10;
            hi=std::max(hi,lo+10);
            p.drawText(12,196,QString("Highest sampled fast: %1 dBA     Highest sampled slow: %2 dBA").arg(std::max_element(fast.begin(),fast.end(),levelLess)->y(),0,'f',1).arg(std::max_element(slow.begin(),slow.end(),levelLess)->y(),0,'f',1));
            QRectF graph(62,260,width-95,350);double duration=std::max(1.,elapsed.elapsed()/1000.);
            p.drawText(12,240,"Level (dBA)");
            for(int i=0;i<=5;++i){
                double frac=i/5.,y=graph.bottom()-frac*graph.height(),x=graph.left()+frac*graph.width();
                p.setPen(Qt::lightGray);p.drawLine(QPointF(graph.left(),y),QPointF(graph.right(),y));
                p.setPen(Qt::black);p.drawText(QPointF(12,y),QString::number(lo+frac*(hi-lo),'f',0));
                p.drawText(QPointF(x-10,graph.bottom()+22),QString::number(duration*frac/60,'f',1));
            }
            auto draw=[&](const QVector<QPointF>&data,QColor color){
                QPainterPath path;
                for(int i=0;i<data.size();++i){
                    QPointF pt(graph.left()+data[i].x()/duration*graph.width(),graph.bottom()-(data[i].y()-lo)/(hi-lo)*graph.height());
                    if(i)path.lineTo(pt);else path.moveTo(pt);
                }
                p.setPen(QPen(color,1.5));p.drawPath(path);if(data.size()==1)p.drawEllipse(path.currentPosition(),2,2);
            };
            draw(fast,QColor("#8859b7"));draw(slow,QColor("#12649c"));
            p.drawText(12,680,"Blue: Slow SPL     Purple: Fast SPL     Horizontal axis: elapsed minutes");
        }else p.drawText(12,210,"No valid level readings were captured.");
    }
    return file.commit();
}
void ServiceDesk::finish(){
    if(!recording())return;
    timer.stop();timeline.close();
    if(!writeReport()){notify("Could not write the report PDF. The recorded timeline is available in the archive.");return;}
    if(!preferences.destination().isEmpty()){
        auto name="Service report "+QFileInfo(folder).fileName()+" "+QUuid::createUuid().toString(QUuid::WithoutBraces).left(8)+".pdf";
        if(!saveBytes(folder+"/folder-pending",name.toUtf8())){notify("PDF archived locally. Could not queue the shared copy.");return;}
        notify(deliverFolder(folder,preferences.destination())?"Report saved to the configured folder.":"PDF archived locally. Waiting for the report folder.");
        return;
    }
    if(preferences.available() && !preferences.automatic()){notify("Report archived. Automatic email is off in Reporting Preferences.");return;}
    QJsonObject job{{"to",preferences.email()},{"subject",activeTitle+" · "+started.date().toString(Qt::ISODate)}};
    if(!saveBytes(folder+"/pending.json",QJsonDocument(job).toJson())){notify("Report archived, but the delivery queue could not be saved.");return;}
    notify("Report queued for Apple Mail.");retry();
}
void ServiceDesk::retry(){
    if(!preferences.destination().isEmpty()){
        for(const auto &entry:archives()){
            const auto path=rootFolder()+"/"+entry;
            if(QFile::exists(path+"/folder-pending") && deliverFolder(path,preferences.destination()))
                notify("Queued report saved to the configured folder.");
        }
        return;
    }
    if(!dispatching.isEmpty()||!preferences.automatic())return;
    auto entries=archives();std::reverse(entries.begin(),entries.end());
    for(const auto &entry:entries){
        auto path=rootFolder()+"/"+entry;
        QFile pending(path+"/pending.json");if(!pending.open(QIODevice::ReadOnly))continue;
        auto job=QJsonDocument::fromJson(pending.readAll()).object();pending.close();
        if(!QFile::exists(path+"/report.pdf"))continue;
        auto lock=std::make_unique<QLockFile>(path+"/delivery.lock");
        lock->setStaleLockTime(0);
        if(!lock->tryLock(0) || !QFile::exists(path+"/pending.json"))continue;
        deliveryLock=std::move(lock);
        dispatching=path;
        mail.start("/usr/bin/osascript",{"-e",QString::fromUtf8(mailScript),preferences.email(),job.value("subject").toString(),path+"/report.pdf"});
        return;
    }
}
void ServiceDesk::shutdown(){
    retryTimer.stop();finish();
    if(!dispatching.isEmpty())mail.waitForFinished(15000);
}
QVariantMap ServiceDesk::room(QUuid id){
    auto s=sources->getByUUid(id);if(!s){return {{"message","Select a measurement with an impulse response."}};}
    std::vector<double> p,t;
    s->lock();for(unsigned i=0;i<s->timeDomainSize();++i){double v=s->impulseValue(i);p.push_back(v*v);t.push_back(s->impulseTime(i)/1000.);}s->unlock();
    if(p.size()<100){return {{"message","Capture a longer impulse response."}};}
    size_t peak=std::max_element(p.begin(),p.end())-p.begin();
    auto tail=p.size()/10;double noise=std::accumulate(p.end()-tail,p.end(),0.)/tail;
    if(p[peak]<=std::max(1e-20,noise*3162)){return {{"message","Insufficient impulse-to-noise ratio for T20."}};}
    double sum=0;for(size_t i=p.size();i-->peak;){sum+=std::max(0.,p[i]-noise);p[i]=sum;}
    double n=0,x=0,y=0,xx=0,xy=0,yy=0;
    for(size_t i=peak;i<p.size();++i){double d=10*std::log10(p[i]/sum),time=t[i]-t[peak];if(d>=-25&&d<=-5){n++;x+=time;y+=d;xx+=time*time;yy+=d*d;xy+=time*d;}}
    double a=n*xy-x*y,b=n*xx-x*x,fit=a*a/(b*(n*yy-y*y));
    if(n<10||!(fit>0.95)||!(a<0&&b>0)){return {{"message","No reliable −5 to −25 dB decay fit. Check routing and impulse length."}};}
    QVariantList points;
    for(size_t i=peak;i<p.size();i+=std::max(size_t(1),(p.size()-peak)/600)){points.append(QPointF(t[i]-t[peak],std::max(-60.,10*std::log10(p[i]/sum))));}
    return {{"points",points},{"message",QString("Broadband RT60 estimate %1 s · T20 fit R² %2").arg(-60*b/a,0,'f',2).arg(fit,0,'f',3)}};
}
