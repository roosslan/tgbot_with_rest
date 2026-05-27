#include "chanserv.h"
#include "support.h"

#include <boost/coroutine2/coroutine.hpp>

using namespace TgBot;

void chanserv_bot::session_dispatcher(){
m_tg_bot.getEvents().onAnyMessage([this](Message::Ptr message)
{
    if (!is_db_consumer(message->from->id))
    {
        c_out << "Клиент " + message->from->username + " (" + itoa(message->from->id) + ") отсутствует в базе, добавляем в его таблицу dbo.consumers" << std::endl;
/*      AddConsumerToDB(&msDatabase, message->from->id, message->from->username);   */
        add_consumer_to_db(&pgDatabase, message->from->id, message->from->username);
        reload_consumers_list();
    }

    std::cout << return_current_time_and_date() << ": Диспетчер: ";

    if(message->animation)
    {
        std::cout << "анимация";
    }
    if(message->audio)
    {
        std::cout << " аудиозапись";
    }
    if(message->contact)
    {
        std::cout << " контакт";
    }
    if(message->document)
    {
        std::cout << " документ " << message->document->fileId;
    }
    if(message->location)
    {
        std::cout << " местоположение";
    }
    if(!message->photo.empty())
    {
        std::cout << " изображение";
    }
    if(message->poll)
    {
        std::cout << " опрос";
    }
    if(message->sticker)
    {
        std::cout << " стикер " + message->sticker->fileId + " ";
    }
    if(message->video)
    {
        std::cout << " видео";
    }
    if(message->voice)
    {
        std::cout << " голосовое";
    }
    std::cout << " от ";

    if (std::find(v_allowed_contractors.begin(), v_allowed_contractors.end(), message->from->id) != v_allowed_contractors.end()) std::cout << "исполнителя ";
/*  if (std::find(allowedConsumers.begin(), allowedConsumers.end(), itoa(message->from->id)) != allowedConsumers.end()) std::cout << " заказчика ";     */

    std::cout << message->from->firstName << " @" << message->from->username << " (" << message->from->id << "), текст: \"" << message->text << "\"" << std::endl;

    /* Если на нового написавшего еще не заведено дело, то добавляем его в массив юзеров, активных в данном запуске бота */
    uD = uniq_data(message->from->id);
    if (!uD)
    {
        uD = new user_dispatcher(message->from->id, message->from->username, message->from->firstName);
        /* uD->jiraTask = new newTask(); */
        v_uD.push_back(uD);
    }
});                                                         };

void chanserv_bot::handle_small_talk()
{
    m_tg_bot.getEvents().onNonCommandMessage([this](Message::Ptr message)
    {
        uD = uniq_data(message->from->id);
        if (uD)
        {
          switch (uD->jira_task.stat_) {
            case revit_master_key_firstname_step2: /* Прислали ФИО для ключа Revit */
            {
                message->text = truncate_msg_from_user(message->text, 3000);
                uD->jira_task.consumer_name = message->text;

                InlineKeyboardMarkup::Ptr kb_reject_or_approve = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
                std::vector<InlineKeyboardButton::Ptr> row_key_request;
                InlineKeyboardButton::Ptr btn_approve_request = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
                btn_approve_request->text = "Одобрить";
                const std::string approve_callback_data = "approve_key_request_" + itoa(message->from->id);
                btn_approve_request->callbackData = approve_callback_data;

                row_key_request.push_back(btn_approve_request);

                InlineKeyboardButton::Ptr btn_reject_request = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
                btn_reject_request->text = "Отказать";
                const std::string reject_callback_data = "_reject_key_request_" + itoa(message->from->id);
                btn_reject_request->callbackData = reject_callback_data;
                row_key_request.push_back(btn_reject_request);

                kb_reject_or_approve->inlineKeyboard.push_back(row_key_request);

                in_thread dialog_with_staff([this, message, kb_reject_or_approve]{ send_msg_to_staff(m_tg_bot.getApi(), v_staff, "Запрос одобрения ключа от " +
                            uD->jira_task.consumer_name +
                            // message->from->firstName + " " + message->from->lastName + " (" + message->from->username + ", " + itoa(message->from->id) + ")" +
                            " Причина: " + uD->jira_task.key_reason, kb_reject_or_approve); });

                m_tg_bot.getApi().sendMessage(message->chat->id, "Ожидайте одобрения");

                /* Чистим статус текущего этапа общения с ботом, иначе 'Ожидайте одобрения' данный блок case будет постоянно повторяться */
                uD->jira_task.stat_ = 0;
                break;
            }
            case revit_master_key_reason_step1: /* Прислали причину запроса ключа для Revit */
            {
                message->text = truncate_msg_from_user(message->text, 3000);
                uD->jira_task.key_reason = message->text;
                m_tg_bot.getApi().sendMessage(message->from->id, "Напишите свои имя и фамилию");
                uD->jira_task.stat_ = revit_master_key_firstname_step2;
                break;
            }
            case staff_add_new_contractor:
            {
                const QStringList contractor_parts = QString::fromStdString(message->text).split("\\");
                /* Добавляем нового исполнителя в БД в таблицу contractors */
                const QString ins_contractors_table = "INSERT INTO contractors (id, name, link, trello_list, email) VALUES ('" + contractor_parts.at(0) + "'," +
                                              "'" + contractor_parts.at(4) + "'," +
                                              "'" + contractor_parts.at(1) + "'," +
                                              "'" + contractor_parts.at(2) + "'," +
                                              "'" + contractor_parts.at(3) + "');";

                QSqlQuery pg_query(pgDatabase);
                c_out << "Adding new contractor into DB from line " + message->text << std::endl;
                pg_query.exec(ins_contractors_table);
                pg_query.next();
                c_out << "pgSqlexec: " << pg_query.lastError().text().toStdString() << std::endl;

                m_tg_bot.getApi().sendMessage(message->chat->id, "Сотрудник " + contractor_parts.at(1).toStdString() + " добвавлен в качестве исполнителя");

                reload_contractors_list();
                uD->jira_task  = {};  /* Очистили структуру */
                break;
            }
/*
            case developer_send_msg_to_gpt:
            {
                in_thread send_msg_to_gpt([this, message] { msg_to_deep_transformer("chatgpt_url", m_tg_bot.getApi(), message->from->id, uD, message->text); });
                break;
            }
*/
            case developer_send_msg_as_bot:
            {
                const long receiverTgID = atol(message->text.substr(0, message->text.find(' ')).c_str());
                const std::string textToSend = message->text.substr(message->text.find(" ") + 1);
                try {
                    m_tg_bot.getApi().sendMessage(receiverTgID, textToSend);
                } catch (TgException e) {
                    m_tg_bot.getApi().sendMessage(message->chat->id, "try/catch: Сообщение " + textToSend + " телеграм юзеру " + itoa(receiverTgID) + " НЕ отправлено");
                }

                m_tg_bot.getApi().sendMessage(message->chat->id, "Сообщение " + textToSend + " телеграм юзеру " + itoa(receiverTgID) + " отправлено");

                uD->jira_task  = {};  /* Очистили структуру */
                break;
            }

            /* Запуск системной команды на хост-компьютере */
            case developer_run_host_command:
            {
                std::string readFromCLI;
                auto ps = new QProcess();

                if (QString::fromStdString(message->text).contains(QChar::Space)) /* Есть аргументы */
                {
                    /* Первое слово выносим в отдельную переменную */
                    const std::string self_the_command = message->text.substr(0, message->text.find(" "));
                    const std::string all_parameters = message->text.substr(message->text.find_first_of(" \t")+1);

                    const QStringList arguments_list = QProcess::splitCommand(QString::fromStdString(all_parameters));
                    ps->start(QString::fromStdString(self_the_command), arguments_list);
                }
                else
                    ps->start(QString::fromStdString(message->text));

                if (ps->waitForStarted(-1)) {
                    while(ps->waitForReadyRead(-1)) {
                        readFromCLI = ps->readAllStandardOutput();
                    }
                    send_msg_to_support(m_tg_bot.getApi(), sett["support_TgID"], readFromCLI, kb_developer);
                }
                /* Ошибка */
                else
                {
                    readFromCLI = ps->readAllStandardOutput();
                    const QString p_stderr = ps->readAllStandardError();
                    send_msg_to_support(m_tg_bot.getApi(), sett["support_TgID"], "Ошибка выполнения команды " + p_stderr.toStdString(), kb_developer);
                }

                delete ps;
                break;
            }

            case staff_set_new_msi_version:
            {
                /* Записываем новую версию .msi в БД */
                const QString updSettingsTable = "UPDATE settings SET value = '" + QString::fromStdString(message->text) + "' WHERE name = 'MSI_actual_version'";
                QSqlQuery qt_query(h_msDB);
                c_out << "Updating .msi version to: " + message->text << std::endl;
                const auto r_ret = qt_query.exec(updSettingsTable);

                m_tg_bot.getApi().sendMessage(message->chat->id, "Версия сборки плагина актуализирована в БД до " + message->text);

                uD->jira_task  = {};  /* Очистили структуру */
                break;
            }            

            case shared_separate_tekla_from_bim_step6:
            {
                if (message->text != "") /* Прислали текст, не аудио-видео etc */
                {
                    /* Прислали имя заказчика */
                    message->text = truncate_msg_from_user(message->text, 150);
                    uD->jira_task.consumer_name = message->text;

                    uD->jira_task.consumer_tg_id = message->chat->id;
                    uD->jira_task.consumer_tg_username_link = message->chat->username;
                    uD->jira_task.stat_ = shared_final_step7;
                    /* без case break - переходим сразу к следующему case */
                }
                else break;
            }
            case shared_final_step7: /* Пора создавать карточку   */
            {
                in_thread createIssue([this, message]{ create_new_issue(message, uD); });
                break;
            }

            case task_descr_step4: /* Нам прислали описание */
            {
                message->text = truncate_msg_from_user(message->text, 3000);
                uD->jira_task.description = message->text;
                cout_small_talk(*uD, "Description", message);
                m_tg_bot.getApi().sendMessage(message->chat->id, "Пришлите изображения и <i><b>после их отправки</b></i> нажмите кнопку \"Нет больше фото\"",
                                          nullptr, nullptr, kb_manage_photos, "Html");
                uD->jira_task.stat_ = shared_firstname_step5;
                break;
            }

            /* Прислали text от клиентов Текла, и мы его записываем в поле Description */
            /* Прислали общую задачу по Текла
            case TEKLA_GENERAL_ISSUE_STEP1:
            {
                if (message->text != "") /* Прислали текст, не аудио-видео etc
                {
                    message->text = std::regex_replace(message->text, std::regex("[\r\t\n]+" ), "");
                    message->text = truncate_msg_from_user(message->text, 3000);
                    uD->jiraTask.description = message->text;

                    uD->jiraTask.ConsumerTgId = message->chat->id;
                    uD->jiraTask.ConsumerTgUsernameLink = message->chat->username;

                    message->text = "Пользователь Tekla";
                    uD->jiraTask.STAT = TASK_DESCR_STEP4;
                    break;
                }
                else break;
            } */

            case task_subj_step3: /* Нам прислали тему */
            {
                message->text = std::regex_replace(message->text, std::regex("[\r\t\n]+" ), "");
                message->text = truncate_msg_from_user(message->text, 150);
                uD->jira_task.subj = message->text;

                cout_small_talk(*uD, "Subject", message);
                m_tg_bot.getApi().sendMessage(message->chat->id, "Опишите максимально подробно проблему");
                uD->jira_task.stat_ = task_descr_step4;
                break;
            }

            case task_project_step2: /*  Нам прислали название проекта */
            {
                message->text = std::regex_replace(message->text, std::regex("[\r\t\n]+" ), "");
                message->text = truncate_msg_from_user(message->text, 150);
                uD->jira_task.project = message->text;
                cout_small_talk(*uD, "Project", message);
                m_tg_bot.getApi().sendMessage(message->chat->id, "Записано\nНапишите одним словом тему обращения");
                uD->jira_task.stat_ = task_subj_step3;
                break;
            }

           case task_section_step1: /* Нам прислали название раздела    */
           {
               message->text = std::regex_replace(message->text, std::regex("[\r\t\n]+" ), "");
               message->text = truncate_msg_from_user(message->text, 150);
                uD->jira_task.section = message->text;

                cout_small_talk(*uD, "Section", message);
                m_tg_bot.getApi().sendMessage(message->chat->id, "Записано\nНапишите, по какому проекту вопрос?\nНапример: Синергия 11.1.3 или Кампусы ВН");
                uD->jira_task.stat_ = task_project_step2;
                break;
            }

            case shared_firstname_step5:
            if (message->text.empty()) /* Нам прислали фото */
            {
                in_thread receivePhoto([this, message]{ receive_photos(message, uD); });
                break;
            }

            case family_project_step4: /* Нам прислали путь (поле project в БД) */
            {
                message->text = std::regex_replace(message->text, std::regex("[\r\t\n]+" ), "");
                message->text = truncate_msg_from_user(message->text, 150);
                uD->jira_task.project = message->text;
                cout_small_talk(*uD, "Path / proj:", message);
                m_tg_bot.getApi().sendMessage(message->chat->id, "Пришлите изображения и <b>после</b> их отправки нажмите кнопку \"Нет больше фото\"",
                                          nullptr, nullptr, kb_manage_photos, "Html");
                uD->jira_task.stat_ = shared_firstname_step5;
                break;
            }

            case  family_descr_step3: /* Нам прислали описание */
            {
               message->text = truncate_msg_from_user(message->text, 3000);
               uD->jira_task.description = message->text;
               cout_small_talk(*uD, "Description", message);
               m_tg_bot.getApi().sendMessage(message->chat->id, "<u><b>Путь/ссылка</b></u> на документацию (сайт или семейство):", nullptr, nullptr, nullptr, "Html");
               uD->jira_task.stat_ = family_project_step4;
               break;
            }

            case family_category_step2: /*   Нам прислали категорию  */
            {
                message->text = std::regex_replace(message->text, std::regex("[\r\t\n]+" ), "");
                message->text = truncate_msg_from_user(message->text, 150);
                uD->jira_task.category = message->text;
                cout_small_talk(*uD, "Category", message);
                m_tg_bot.getApi().sendMessage(message->chat->id, "<u><b>Напиши, что нужно сделать</b></u>:\nЖелательно указать:\n -параметры;\n -информацию об УГО", nullptr, nullptr, nullptr, "Html");
                uD->jira_task.stat_ = family_descr_step3;
                break;
            }

            case family_unit_step1: /* Нам прислали раздел */
            {
                message->text = std::regex_replace(message->text, std::regex("[\r\t\n]+" ), "");
                message->text = truncate_msg_from_user(message->text, 150);

                uD->jira_task.section = message->text;
                cout_small_talk(*uD, "Section", message);
                m_tg_bot.getApi().sendMessage(message->chat->id, "Записано\n<u><b>Категория</b></u> семейства: ", nullptr, nullptr, nullptr, "Html");
                uD->jira_task.stat_ = family_category_step2;
                break;
            }

            default:
               break;
            }
        }
    });
}

