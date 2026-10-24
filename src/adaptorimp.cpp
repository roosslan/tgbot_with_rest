#include "adaptorimp.h"
#include <iostream>

AdaptorImp::AdaptorImp(QObject *parent) : QObject(parent)
{ }

void AdaptorImp::send_tg_message(const QString &message)
{
    std::cout << return_current_time_and_date() << ": ListChecker: " << message.toStdString() << std::endl;
}

void AdaptorImp::run_command(const QString &cmd, const QVariantMap &customdata)
{
    /* Команду не выполняем: ListChecker перезапускает supervisord, а выполнение произвольной команды по DBus небезопасно */
    Q_UNUSED(customdata);
    std::cout << return_current_time_and_date() << ": ListChecker запросил перезапуск (" << cmd.toStdString() << ")" << std::endl;
}
