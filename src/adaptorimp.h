#pragma once

#ifndef ADAPTORIMP_H
#define ADAPTORIMP_H

#include <QObject>
#include <QtDBus/QtDBus>
#include "support.h"

// Object that implement the funcionality of the adaptor
class AdaptorImp : public QObject
{
    Q_OBJECT
public:
    explicit AdaptorImp(QObject *parent = 0);
public Q_SLOTS:
    void send_tg_message(const QString &message);
    void run_command(const QString &cmd, const QVariantMap &customdata);
Q_SIGNALS:
    void LateEvent(const QString &eventkind);
signals:

public slots:

};

#endif
