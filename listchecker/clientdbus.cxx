
#include "clientdbus.hpp"

/*
 * Implementation of interface class ClientDBus
 */

client_dbus::client_dbus(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(service, path, static_interface_name(), connection, parent)
{
}

client_dbus::~client_dbus()
{
    /*  */
}

