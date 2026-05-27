#include <iostream>
#include <unistd.h>
#include <csignal>

#include <tgbot/tgbot.h>
#include <tgbot/Bot.h>
#include "../src/support.h"
#include "listchecker.hpp"

int main(int argc, char *argv[])
{
    /* QSqlDatabase requires a QCoreApplication */
    QCoreApplication app(argc, argv);
/*  QString configFile = qgetenv("CHANSERV_CONFIG"); */

    /* Данный таймер нужен для того, чтобы сообщать разработчику об аптайме и об ошибке связи с БД, но не спамить безостановочно, а сообщать, скажем, раз в 20 минут */
    QElapsedTimer* up_timer = new QElapsedTimer();
    up_timer->start();

    qInstallMessageHandler(bg_message_handler); /* Handler, чтобы qDebug() писал свой вывод в файл */

    list_checker list_helper;

    /* Initializing JIRA */
    list_helper.initialize_tracking_apis();

    /* Бесконечный цикл checkTheList() */
    c_out << "Проверка карточек будет повторяться каждые " + itoa(list_helper.LISTCHECK_DURATION_IN_MSEC) + " миллисекунд" << std::endl;
    active_object obj([&list_helper, up_timer]
                      {
                          QEventLoop loop;
                          list_helper.check_the_list(up_timer);
                          QTimer::singleShot(list_helper.LISTCHECK_DURATION_IN_MSEC, &loop, SLOT(quit()));
                          loop.exec();
                      });

    /* Бесконечный служебный цикл */
    c_out << "Профилирование будет повторяться каждые 3.5 часа" << std::endl;
    active_object bgo([&list_helper, up_timer]
                      {
                          QEventLoop loop;
                          list_helper.nix_bg_worker(up_timer);
                          QTimer::singleShot(12600000, &loop, SLOT(quit()));
                          loop.exec();
                      });

    return app.exec();
}
