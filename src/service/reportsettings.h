// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QJsonObject>

class ReportSettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString email READ email NOTIFY changed)
    Q_PROPERTY(bool automatic READ automatic NOTIFY changed)
    Q_PROPERTY(bool configured READ configured NOTIFY changed)
public:
    ReportSettings();
    QString email() const;
    bool automatic() const;
    bool available() const { return accessible; }
    bool configured() const;
    Q_INVOKABLE QString save(QString email, bool automatic, QString pin, QString confirmation);
signals:
    void changed();
private:
    bool reload();
    QJsonObject record;
    bool accessible = false;
};
