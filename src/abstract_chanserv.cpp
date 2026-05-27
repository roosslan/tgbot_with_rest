#include <iostream>
#include "abstract_chanserv.h"

AbstractChanserv::AbstractChanserv()
{
    c_out << "AbstractChanserv: Start SELECTing arrays from the database" << std::endl;

    QSqlQuery pg_query(pgDatabase);
    /* Сотрудники, которым необходимо получать служебные уведомления */
    pg_query.clear();
    pg_query.exec("SELECT id FROM staff");
    while(pg_query.next())
        v_staff.push_back(pg_query.value(0).toLongLong());

    pg_query.clear();
    pg_query.exec("SELECT id FROM stickers");
    while(pg_query.next())
        sticker_list.push_back(pg_query.value(0).toString().toStdString());

    reload_contractors_list();

    reload_consumers_list();
}

int AbstractChanserv::get_mem_size(const std::string& script_path)
{
#if defined(Q_OS_LINUX)
        QProcess p;
        p.start("/bin/sh", QStringList() << QString::fromStdString(script_path));
        p.waitForFinished();
        const QString qStdout = p.readAllStandardOutput();
        const std::string s_stdout = qStdout.toStdString();

        /* Триммим строку "  19.60 Mb /home/username/chanserv/bin/chanserv  ", чтобы остался только размер занятой памяти */
        std::string chanserv_memSize = s_stdout.substr(0, s_stdout.find("Mb"));
        boost::trim_right(chanserv_memSize);
        boost::trim_left(chanserv_memSize);

        p.close();

        std::string::size_type sz;     // alias of size_t
        const float f_MemSize = std::stof(chanserv_memSize, &sz);
        const int int_rret = (int)f_MemSize;
        return int_rret;
#endif
    return 0;
}

void AbstractChanserv::reload_contractors_list()
{
    c_out << "Reloading contractors list" << std::endl;
    clear_stl_container(v_contractors);
    QSqlQuery qt_query(pgDatabase);
    qt_query.clear();
    qt_query.exec("SELECT id, name, link, trello_list FROM contractors");
    contractor item_contractor;
    while(qt_query.next())
    {
        item_contractor.id = qt_query.value(0).toLongLong();
        item_contractor.name = qt_query.value(1).toString().toStdString();
        item_contractor.link = qt_query.value(2).toString().toStdString();
        item_contractor.trello_list = qt_query.value(3).toString().toStdString();
        v_allowed_contractors.push_back(item_contractor.id);
        v_contractors.push_back(item_contractor);
        item_contractor = {};                    /* Очищаем структуру */
    }
}

void AbstractChanserv::reload_consumers_list()
{
    c_out << "Reloading consumers list" << std::endl;
    clear_stl_container(v_consumers);
    QSqlQuery qt_query(pgDatabase);
    qt_query.clear();
    qt_query.exec("SELECT id FROM consumers");
    while(qt_query.next())
        v_consumers.push_back(qt_query.value(0).toLongLong());
}
