// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QJsonObject>

class ReportSettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString destination READ destination NOTIFY changed)
    Q_PROPERTY(QString email READ email NOTIFY changed)
    Q_PROPERTY(bool automatic READ automatic NOTIFY changed)
    Q_PROPERTY(bool configured READ configured NOTIFY changed)
public:
    ReportSettings();
    QString email() const;
    QString destination() const { return record.value("folder").toString(); }
    Q_INVOKABLE QString pickFolder(QString current);
    bool automatic() const;
    bool available() const { return accessible; }
    bool configured() const;
    Q_INVOKABLE QString save(QString email, bool automatic, QString pin, QString confirmation, QString folder = {});
signals:
    void changed();
private:
    bool reload();
    QJsonObject record;
    bool accessible = false;
};
