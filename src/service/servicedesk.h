// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QDialog>
#include <QFile>
#include <QTimer>
#include <deque>
#include "sourcelist.h"
class QLineEdit; class QSpinBox; class QLabel; class QPushButton; class QComboBox;
class ServiceDesk : public QDialog {
    Q_OBJECT
public:
    explicit ServiceDesk(SourceList *sources);
    ~ServiceDesk() override;
    Q_INVOKABLE void present();
private:
    void start();
    void finish();
    void sample();
    void loadReport();
    void room();
    SourceList *sources;
    QUuid sourceId;
    QTimer timer;
    QFile timeline;
    QString folder, summary, activeTitle;
    QLineEdit *title, *recipient;
    QComboBox *inputs, *history;
    QSpinBox *minutes, *yellow, *red;
    QLabel *status, *report;
    QPushButton *record;
    std::deque<double> powers;
    double total=0, windowEnergy=0, maximum=-200;
    int count=0;
};
