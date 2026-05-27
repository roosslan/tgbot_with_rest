#include "serverdbus.h"
#include <QtCore/QMetaObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

/*
 * Implementation of adaptor class ServerDBus
 */

// void ServerDBus::Aborted(){  /* */  }

ServerDBus::ServerDBus(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    // constructor
    setAutoRelaySignals(true);
}

ServerDBus::~ServerDBus()
{
    // destructor
}

void ServerDBus::sendTgMessage(const QString &message)
{
    // handle method call chanserv.qdbus.sendTgMessage
    QMetaObject::invokeMethod(parent(), "sendTgMessage", Q_ARG(QString, message));
}

void ServerDBus::RunCommand(const QString &cmd, const QVariantMap &customdata)
{
    // handle method call chanserv.qdbus.RunCommand
    QMetaObject::invokeMethod(parent(), "RunCommand", Q_ARG(QString, cmd), Q_ARG(QVariantMap, customdata));
}

