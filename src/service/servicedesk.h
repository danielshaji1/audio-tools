// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "reportsettings.h"
#include "sourcelist.h"
#include <QFile>
#include <QTimer>
#include <QElapsedTimer>
#include <QPointF>
#include <QProcess>
#include <QLockFile>
#include <memory>

class ServiceDesk : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool recording READ recording NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QVariantList levels READ levels NOTIFY changed)
    Q_PROPERTY(QStringList archives READ archives NOTIFY changed)
public:
    explicit ServiceDesk(SourceList *sources);
    ReportSettings preferences;
    bool recording() const {return timeline.isOpen();}
    QString status() const {return message;}
    QVariantList levels() const;
    QStringList archives() const;
    Q_INVOKABLE void start(QString title,QUuid id);
    Q_INVOKABLE void finish();
    Q_INVOKABLE QVariantMap room(QUuid id);
    Q_INVOKABLE void openArchive(QString archive);
    Q_INVOKABLE void retry();
    void shutdown();
signals:
    void changed();
private:
    void sample();
    void notify(QString text){message=text;emit changed();}
    bool writeReport();
    SourceList *sources;
    QUuid sourceId;
    QTimer timer,retryTimer;
    QElapsedTimer elapsed;
    QFile timeline;
    QString folder,activeTitle,inputName,message,dispatching;
    QVector<QPointF> slow,fast;
    QDateTime started;
    std::unique_ptr<QLockFile> deliveryLock;
    QProcess mail;
};
