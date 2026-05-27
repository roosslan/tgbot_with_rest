#include "chanserv.h"
#include "support.h"

void chanserv_bot::on_callback_query()
{
    m_tg_bot.getEvents().onCallbackQuery([this](CallbackQuery::Ptr query)
    {
        if (query->data.starts_with("approve_key_request") || query->data.starts_with("_reject_key_request"))
        {
            std::int64_t requester_id = 0;
            const int len = query->data.size();
            try
            {
                requester_id = std::stol(query->data.substr(20, len - 20).c_str());
                uD = uniq_data(requester_id);
                if (requester_id)
                    if (query->data.starts_with("approve_key_request"))
                    {
                        if (uD->jira_task.key_reason != "")
                        {
                            /* InsertRevitKeyToDB проверяет, что в таблице не существует открытых запросов на разблокировку/одобрение
                             * но если такой есть то вернет уже существующий, иначе сгенерирует и запишет новый */
                            auto new_master_key = insert_revit_key_to_db(&h_msDB, uD);
                            if (new_master_key == "")
                                send_msg_to_support(m_tg_bot.getApi(), sett["support_TgID"], "Ошибка генерации мастер ключа Revit для " + itoa(requester_id));
                            else {
                                m_tg_bot.getApi().sendMessage(requester_id, "Ваш запрос ключа для Revit одобрен. Ключ ```" + new_master_key + "```",
                                                         nullptr, nullptr, kb_ganzel, "HTML");
                                in_thread notify_staff([this, query]{ send_msg_to_staff(m_tg_bot.getApi(), v_staff,  "Тов. " + query->from->firstName + " (" +
                                                        query->from->username + ") одобрил запрос от " + uD->jira_task.consumer_name); });                            
                            }
                            /* Clearing request */
                            uD->jira_task.key_reason = "";
                        }
                    }
            }
            catch (std::exception) { }

            if (requester_id)
                if (query->data.starts_with("_reject_key_request"))
                {
                    if (uD->jira_task.key_reason != "")
                    {
                        m_tg_bot.getApi().sendMessage(requester_id, "Ваш запрос ключа для Revit отклонён");
                        in_thread notify_staff([this, query]{ send_msg_to_staff(m_tg_bot.getApi(), v_staff,  "Тов. " + query->from->firstName + " (" +
                                                query->from->username + ") отклонил запрос от " + uD->jira_task.consumer_name); });
                        /* Clearing request */
                        uD->jira_task.key_reason = "";
                    }
                }
        }

        /* Ф-ция uniqData возвращает все данные написавшего в чат юзера: new_task, issues, recent messages etc */
        uD = uniq_data(query->from->id);
        if (uD)
        {
            /* Чтобы не получить бан "429 too many requests", фильтруем - не нажал ли юзер старые инлайновые кнопки со старых бесед с ботом */
            const pgdb_issue *get_clicked_issue = get_selected_issue_id(query->message->messageId, uD->pgdb_issues, uD->recent_msg_ids, uD->recent_msgs);

            /* Запросили ключ для разблокировки Revit */
            if (query->data == "req_master_key" && uD->clicked_msg_id == query->message->messageId)  /* Старые инлайновые нажатия !clickedMsgId не обрабатываем */
            {
                /* Clear the struct */
                uD->jira_task = {};
                m_tg_bot.getApi().sendMessage(query->message->chat->id, "Введите причину");
                uD->jira_task.stat_ = revit_master_key_reason_step1;
            }

            if (query->data == "msg_as_bot")
            {
                m_tg_bot.getApi().sendMessage(query->message->chat->id, "Пришлите Telegram ID (тип long) получателя, ПРОБЕЛ после ID, и текст для отправки после упомянутого пробела");
                uD->jira_task.type = "DEVELOPER";
                uD->jira_task.stat_ = developer_send_msg_as_bot;
            }

            if (query->data == "msg_to_gpt")
            {
                m_tg_bot.getApi().sendMessage(query->message->chat->id, "Для выхода из режима GTP-4o нажмёте на команду /start");
                m_tg_bot.getApi().sendMessage(query->message->chat->id, "Пришлите текст для отправки GPT (Generative pre-trained transformer)");
                uD->jira_task.type = "DEVELOPER";
                uD->jira_task.stat_ = developer_send_msg_to_gpt;
            }

            if (query->data == "run_command")
            {
                m_tg_bot.getApi().sendMessage(query->message->chat->id, "Пришлите команду для выполнения на хосте");
                uD->jira_task.type = "DEVELOPER";
                uD->jira_task.stat_ = developer_run_host_command;
            }

            if (query->data == "reboot_host")
            {
                m_tg_bot.getApi().sendMessage(query->message->chat->id, "Перезагружаем компьютер...");
                const auto ps = new QProcess();
                ps->start(QString::fromStdString(sett["script_path_reboot"]));
                /* Компьютер ушёл на перезагрузку */
            }

            if (query->data == "add_new_contractor" && uD->clicked_msg_id == query->message->messageId)  /* Старые инлайновые нажатия !clickedMsgId не обрабатываем */
            {
                m_tg_bot.getApi().sendMessage(query->message->chat->id,
                    "Введите данные исполнителя в формате\ntelegram-id\\telegram-link\\Trello-список-id\\E-mail\\ФИО исполнителя,\nнапример,\n9275911689\\OctavianAvg\\68033c03d907d7047b2b8f04\\oleg@example.ru\\Олег Булатов");
                uD->jira_task.type = "STAFF";
                uD->jira_task.stat_ = staff_add_new_contractor;
            }

            if (query->data == "set_new_msi_version" && uD->clicked_msg_id == query->message->messageId)  /* Старые инлайновые нажатия !clickedMsgId не обрабатываем */
            {
                m_tg_bot.getApi().sendMessage(query->message->chat->id, "Пришлите текст в формате X.X.XXX.25, например, 5.4.168.25");
                uD->jira_task.type = "STAFF";
                uD->jira_task.stat_ = staff_set_new_msi_version;
            }

            if (query->data == "send_file")  /* старые инлайновые нажатия !clickedMsgId не обрабатываем */
            {
                in_thread send_document_all([this] { send_document_to_all(m_tg_bot.getApi(), v_consumers, "https://www.oexxxxxxxxxxxxx.ru/instruksiya_xxxx_xxx_helper.pdf",
                                  "Здравствуйте!\n\nНаправляем инструкцию по использованию бота.\n\nС уважением,\nадминистрация"); });
            }

            if (query->data == "no_more_photos" && uD->jira_task.stat_ == shared_firstname_step5)           /* Нет больше фото */
            {
                c_out << "Callback от " << itoa(query->message->chat->id) << " \"Нет больше фото\"" << std::endl;
                m_tg_bot.getApi().sendMessage(query->message->chat->id, "Отлично\nНапишите свои имя и фамилию", nullptr, nullptr, kb_ganzel);
                uD->jira_task.stat_ = shared_separate_tekla_from_bim_step6;
            }

            /* МОИ ЗАДАЧИ ****************************************************************** / kbInlineContractor / *****************/

            if (query->data == "my_tasks" && uD->clicked_msg_id == query->message->messageId)  /* старые инлайновые нажатия !clickedMsgId не обрабатываем */
            {
                uD->jira_task = {};          /* Clear the struct  */
                uD->clicked_msg_id = 0;
                in_thread get_issues([this, &query]{ get_my_issues(query->from->id); });
            }


            /* TEKLA ****************************************************************** / kbInlineTekla / *****************/

            if (query->data == "tekla_enter_menu" && uD->clicked_msg_id == query->message->messageId)  /* старые инлайновые нажатия !clickedMsgId не обрабатываем */
            {
                /* Изменяем текст кнопки "Текла" */
                m_tg_bot.getApi().editMessageReplyMarkup(query->message->chat->id, query->message->messageId, "", kb_tekla);
                uD->jira_task.subj = "Tekla";
            }

            if (query->data == "tekla_general_issue")
            {
                /* Убираем "Текла"-кнопки  */
                m_tg_bot.getApi().deleteMessage(query->message->chat->id, query->message->messageId);

                m_tg_bot.getApi().sendMessage(query->message->chat->id, "Напишите одним словом тему обращения");
                uD->jira_task.type = "TEKLA";
                uD->jira_task.tekla_card_caption = uD->jira_task.section = "Проблемы";
                uD->jira_task.stat_ = task_subj_step3;
            }
/*
            if ((query->data == "tekla_model_repair"))
            {
                if (!chatSessions->hasSession(query->from->id)) {
                    c_out << "Creating new chat session for tg user " << itoa(query->from->id) << std::endl;
                    chatSessions->createNewTgSession(query->from->id);
                }
                chatSessions->passMessage(query->from->id, query->message->text);

                // Убираем "Текла"-кнопки
                m_tgBot.getApi().deleteMessage(query->message->chat->id, query->message->messageId);

                m_tgBot.getApi().sendMessage(query->message->chat->id, "Впишите путь модели:");
                uD->jiraTask.type = "TEKLA";
                uD->jiraTask.cardCaption = uD->jiraTask.section = "Поломка модели";
                uD->jiraTask.STAT = TEKLA_REPAIR_MODEL_STEP1;
            }
*/

            if (query->data == "tekla_back" && uD->clicked_msg_id == query->message->messageId)  /* старые инлайновые нажатия !clickedMsgId не обрабатываем */
            {
                /* Возвращаемся в главное меню кнопок */
                m_tg_bot.getApi().editMessageReplyMarkup(query->message->chat->id, query->message->messageId, "", kb_consumer);
                uD->jira_task = {};              /* Clear the struct  */
            }

            /* ПРИСТУПИТЬ К ВЫПОЛНЕНИЮ *********************************************************************************************************************/

            if ( (query->data == "start_solving") && get_clicked_issue != nullptr /* kb_inline_start_solving */
                    && uD->clicked_msg_id == 0)       /* Эта проверка добавлена чтобы не срабатывали многкратные нажатия на кнопку "Приступить к выполнению", т.к. выбросывается TgException */
                         /* Bad Request: message is not modified: specified new message content and reply markup are exactly the same as a current content and reply markup of the message */
            {
                /* Изменяем текст кнопки, прикрепленной к задаче - с "Приступить" на "Завершить" */
                try
                {
                    m_tg_bot.getApi().editMessageReplyMarkup(query->message->chat->id, query->message->messageId, "", kb_task_closing);
                    uD->clicked_msg_id = query->message->messageId;
                } catch (TgException e) { }

                /* MessageBox: */
                m_tg_bot.getApi().answerCallbackQuery(query->id, "Вы приступили к выполнению задачи.\nПосле завершения задачи, задача будет недоступна, и отправится в архив", true);

                /* Изменяем цвет карточки в Trello */
                m_trello_api->change_card_color(get_clicked_issue->id_card, "green");
            }

            /* НОВАЯ ЗАДАЧА ************************************************************************************************************************/

            if(query->data == "new_task")
            {
                uD = uniq_data(query->from->id);
                if (uD)
                {
                    if (uD->clicked_msg_id != query->message->messageId)  /* старые инлайновые нажатия !clickedMsgId не обрабатываем */
                        return;
                    uD->clicked_msg_id = 0;

                    uD->jira_task = {};          /* Clear the struct  */
                    /* Обрабатываем запросы - только, если это наш сотрудник */
/*                  if (isAllowedConsumer(query->from->id) || isAllowedContractor(query->from->id))                                 */
                    {
                        c_out << query->message->chat->username + " начал заполнять заявку на новую задачу." << std::endl;
                        uD->jira_task.type = "BIMTASK";
                        on_new_task_query(query);
                    }
                }
            }

            /* СЕМЕЙСТВА **************************************************************************************************************************/

            if(query->data == "new_family")
            {
                uD = uniq_data(query->from->id);
                if (uD)
                {
                    if (uD->clicked_msg_id != query->message->messageId)  /* старые инлайновые нажатия !clickedMsgId не обрабатываем */
                        return;
                    uD->clicked_msg_id = 0;

                    uD->jira_task = {};          /* Clear the struct  */
                    /* Обрабатываем запросы - только, если это наш сотрудник */
/*                  if (isAllowedConsumer(query->from->id) || isAllowedContractor(query->from->id))                                 */
                    {
                        c_out << query->message->chat->username + " начал заполнять заявку на новое семейство." << std::endl;
                        uD->jira_task.subj = "Создать семейство";
                        uD->jira_task.type = "FAMILY";
                        on_new_family_query(query);
                    }
                }
            }

            if (query->data == "more_photos" && get_clicked_issue != nullptr)           /* kb_inline_start_solving_w_photos */
            {
                on_more_photos_command(*get_clicked_issue, query);
            }


            /* === TASK_CLOSING ======================================================================================================================= */

            /* Чтобы не получить бан "429 too many requests", фильтруем - не нажал ли юзер старые инлайновые кнопки со старых бесед с ботом */
            if (query->data == "task_closing" && get_clicked_issue != nullptr &&              /* kbInlineTaskClosing */
                    uD->clicked_msg_id != 0)       /* Эта проверка добавлена чтобы не срабатывали многкратные нажатия на кнопку "Завершить выполнение", т.к. выбросывается TgException */
            /* Bad Request: message is not modified: specified new message content and reply markup are exactly the same as a current content and reply markup of the message */
            {
                c_out << itoa(uD->clicked_msg_id) << " TC: MsgID " << itoa(query->message->messageId) << " QueryID " << query->id << std::endl;
                /* Изменяем текст кнопки, прикрепленной к задаче - с "Завершить" на "Выполнена" */
                m_tg_bot.getApi().editMessageReplyMarkup(query->message->chat->id, query->message->messageId, "", kb_task_done);
                uD->clicked_msg_id = 0;
                /* MessageBox: */
                m_tg_bot.getApi().answerCallbackQuery(query->id, "Вы завершили задачу\nМожно приступать к выполнению следующей", true);

                /* Уведомление о выполнении задачи для админ.аппарата */
                in_thread notify_staff([this, get_clicked_issue]{ send_msg_to_staff(m_tg_bot.getApi(), v_staff,  "Исполнитель " + get_clicked_issue->contractor_name + " @" + get_clicked_issue->contractor_link + \
                                        "\nвыполнил задачу " +
                                        itoa(get_clicked_issue->id) + " " +
                                        get_clicked_issue->issue_num + " " +
                                        get_clicked_issue->theme + " от пользователя\n" + get_clicked_issue->from, kb_go_dashboard); });

                /* Архивируем карточку на доске Trello */
                m_trello_api->archiving_card(get_clicked_issue->id_card);

                /* И удаляем карточку из нашей БД */
                if (delete_issue_from_db(&pgDatabase, QString::number(get_clicked_issue->id)))
                {
                    std::cout << "Карточка " << get_clicked_issue->id << " удалена из БД" << std::endl;
                    delete_issue_from_db(&pgDatabase, QString::number(get_clicked_issue->id));
                }
            }
        }
    });
}

void chanserv_bot::on_new_task_query(CallbackQuery::Ptr query)
{
    uD = uniq_data(query->from->id);
    if (uD)
    {
        m_tg_bot.getApi().sendMessage(query->message->chat->id, "Напишите свой раздел", nullptr, nullptr, kb_ganzel);
        uD->jira_task.stat_ = task_section_step1;
    }
}

/* Creation new Revit family */
void chanserv_bot::on_new_family_query(CallbackQuery::Ptr query)
{
    uD = uniq_data(query->from->id);
    if (uD)
    {
        m_tg_bot.getApi().sendMessage(query->message->chat->id, "<u><b>Раздел</b></u> семейства:",
                                   nullptr, nullptr, kb_ganzel, "HTML");
        uD->jira_task.stat_ = family_unit_step1;
    }
}

