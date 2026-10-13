#ifndef CHANCONF_H
#define CHANCONF_H

#include "support.h"
#include "structures.h"

#include <toml++/toml.hpp>

/* Значение [section] key из TOML в виде строки (строки и целые числа), иначе default_value */
std::string toml_value_as_string(const toml::table& tbl, std::string_view section, std::string_view key, const std::string& default_value = "");

#include <qsqldatabase.h>
#include <qsqlquery.h>
#include <qsqlerror.h>

/*
 * Interface - a C++ class with only pure virtual methods (i.e. without any code = 0),
 * and an abstract class - a C++ class with virtual methods that can be overridden,
 * and some code, but at least one pure virtual method that makes the class not instantiable.
 */
class IChanserv : public QObject
{
    Q_OBJECT
public:
    virtual ~IChanserv() = default;
    virtual void IInitializeConfig() = 0; // IOnlyPureVirtualMethods() = 0;
    virtual int  IInitializeDB() = 0;
};


class ChanConf : public IChanserv
{
    /* Флаг, если в конструкторе будет ошибка чтения конфига */
    bool isValid = true;

    void IInitializeConfig();
    int  IInitializeDB();
    void read_settings_from_db();

protected:

/*  std::string m_tgBotToken;
    std::string maxBotToken;    */

    const std::string config_file_ = qgetenv("CHANSERV_CONFIG").toStdString();
    toml::table chanserv_toml_;

    /* Значение из chanserv.toml в виде строки (строки и целые числа), иначе default_value */
    std::string conf_value(std::string_view section, std::string_view key, const std::string& default_value = "") const;

    /* Совместная БД MSSQL Server для хранения данных addin-related */
    QSqlDatabase h_msDB  = QSqlDatabase::addDatabase("QODBC", "msiDB");
    std::map<std::string, std::string> msdb;

    QSqlDatabase pgDatabase = QSqlDatabase::addDatabase("QPSQL", "chanserv");
    std::map<std::string, QString> pg;

public:
    ChanConf();

    std::map<std::string, std::string> sett;
    std::string get_setting(const std::string& field_name);
    bool is_valid() const;
};

#endif // CHANCONF_H
