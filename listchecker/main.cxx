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

    /* Проверка карточек и профилирование выполняются таймерами в главном цикле событий:
     * Telegram API и БД используются только из главного потока, без параллельных обращений */
    c_out << "Проверка карточек будет повторяться каждые " + itoa(list_helper.LISTCHECK_DURATION_IN_MSEC) + " миллисекунд" << std::endl;
    QTimer check_timer;
    QObject::connect(&check_timer, &QTimer::timeout, [&list_helper, up_timer] { list_helper.check_the_list(up_timer); });
    check_timer.start(list_helper.LISTCHECK_DURATION_IN_MSEC);

    c_out << "Профилирование будет повторяться каждые 3.5 часа" << std::endl;
    QTimer bg_timer;
    QObject::connect(&bg_timer, &QTimer::timeout, [&list_helper, up_timer] { list_helper.nix_bg_worker(up_timer); });
    bg_timer.start(12600000);

    /* Первые проверки - сразу после старта */
    QTimer::singleShot(0, [&list_helper, up_timer] {
        list_helper.check_the_list(up_timer);
        list_helper.nix_bg_worker(up_timer);
    });

    /* SIGTERM/SIGINT (например, от supervisord) завершают цикл событий */
    std::signal(SIGINT, [](int) { request_stop(); });
    std::signal(SIGTERM, [](int) { request_stop(); });
    QTimer stop_check;
    QObject::connect(&stop_check, &QTimer::timeout, &app, [] { if (stop_requested()) QCoreApplication::quit(); });
    stop_check.start(500);

    return app.exec();
}
