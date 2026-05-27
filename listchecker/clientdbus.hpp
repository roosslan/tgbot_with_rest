#ifndef CLIENTDBUS_H_
#define CLIENTDBUS_H_

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

/*
 * Proxy class for interface chanserv.qdbus
 */
class client_dbus: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *static_interface_name()
    { return "chanserv.qdbus"; }

    typedef std::shared_ptr<client_dbus> Ptr;
    client_dbus(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = nullptr);

    ~client_dbus();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<> send_tg_message(const QString &message)
    {
        QList<QVariant> argumentList;
        argumentList << (QVariant)(message);
        return asyncCallWithArgumentList(QLatin1String("sendTgMessage"), argumentList);
    }

    inline QDBusPendingReply<> run_command(const QString &cmd, const QVariantMap &customdata)
    {
        QList<QVariant> argumentList;
        argumentList << (QVariant)(cmd) << (QVariant)(customdata);
        return asyncCallWithArgumentList(QLatin1String("RunCommand"), argumentList);
    }

Q_SIGNALS: // SIGNALS
    void LateEvent(const QString &eventkind);
};

namespace chanserv {
    typedef ::client_dbus qdbus;
}

#endif
