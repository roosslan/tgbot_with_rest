#pragma once

#ifndef SERVERDBUS_H_1323265714
#define SERVERDBUS_H_1323265714

#include <QtCore/QObject>
#include <QtDBus/QtDBus>

class QByteArray;
template<class T> class QList;
template<class Key, class Value> class QMap;
class QString;
/* class QStringList; */
class QVariant;

/*
 * Adaptor class for interface chanserv.qdbus
 */
class ServerDBus: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "chanserv.qdbus")
    Q_CLASSINFO("D-Bus Introspection", ""
        "  <interface name=\"chanserv.qdbus\">\n"
        "    <method name=\"RunCommand\">\n"
        "      <annotation value=\"QVariantMap\" name=\"com.chanserv.QtDBus.QtTypeName.In1\"/>\n"
        "      <arg direction=\"in\" type=\"s\" name=\"cmd\"/>\n"
        "      <arg direction=\"in\" type=\"a{sv}\" name=\"customdata\"/>\n"
        "    </method>\n"
        "    <method name=\"sendTgMessage\">\n"
        "      <arg direction=\"in\" type=\"s\" name=\"message\"/>\n"
        "    </method>\n"
        "    <signal name=\"LateEvent\">\n"
        "      <arg direction=\"out\" type=\"s\" name=\"eventkind\"/>\n"
        "    </signal>\n"
        "  </interface>\n"
    "")

public:
    ServerDBus(QObject *parent);
    virtual ~ServerDBus();
public: /* PROPERTIES*/


public slots:
    // Q_NOREPLY void Aborted();
public Q_SLOTS: // METHODS
    void sendTgMessage(const QString &message);
    void RunCommand(const QString &cmd, const QVariantMap &customdata);
Q_SIGNALS: // SIGNALS
    void LateEvent(const QString &eventkind);
};

#endif
