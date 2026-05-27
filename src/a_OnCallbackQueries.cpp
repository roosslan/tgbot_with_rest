#include "chanserv.h"
#include "support.h"

void ChanservBot::OnCallbackQuery()
{
    m_tgBot.getEvents().onCallbackQuery([this](CallbackQuery::Ptr query)
    {
        if (query->data.starts_with("approve_key_request") || query->data.starts_with("_reject_key_request"))
        {
            std::int64_t RequesterID = 0;
            int len = query->data.size();
            try
            {
                RequesterID = std::stol(query->data.substr(20, len - 20).c_str());
                uD = uniqData(RequesterID);
                if (RequesterID)
                    if (query->data.starts_with("approve_key_request"))
                    {
                        if (uD->jiraTask.keyReason != "")
                        {
                            /* InsertRevitKeyToDB проверяет, что в таблице не существует открытых запросов на разблокировку/одобрение
                             * но если такой есть то вернет уже существующий, иначе сгенерирует и запишет новый */
                                m_tgBot.getApi().sendMessage(RequesterID, "Ваш запрос ключа для Revit одобрен. Ключ ```" + itoa(InsertRevitKeyToDB(&msDB, uD)) + "```",
                                                         nullptr, nullptr, kbGanzel, "HTML");
                            inThread NotifyStaff([this, query]{ SendMsgToStaff(m_tgBot.getApi(), Staff,  "Тов. " + query->from->firstName + " (" +
                                                        query->from->username + ") одобрил запрос от " + uD->jiraTask.ConsumerName); });
                            /* Clearing request */
                            uD->jiraTask.keyReason = "";
                        }
                    }
            }
            catch (std::exception) { }

            if (RequesterID)
                if (query->data.starts_with("_reject_key_request"))
                {
                    if (uD->jiraTask.keyReason != "")
                    {
                        m_tgBot.getApi().sendMessage(RequesterID, "Ваш запрос ключа для Revit отклонён");
                        inThread NotifyStaff([this, query]{ SendMsgToStaff(m_tgBot.getApi(), Staff,  "Тов. " + query->from->firstName + " (" +
                                                query->from->username + ") отклонил запрос от " + uD->jiraTask.ConsumerName); });
                        /* Clearing request */
                        uD->jiraTask.keyReason = "";
                    }
                }
        }

        /* Ф-ция uniqData возвращает все данные написавшего в чат юзера: new_task, issues, recent messages etc */
        uD = uniqData(query->from->id);
        if (uD)
        {
            /* Чтобы не получить бан "429 too many requests", фильтруем - не нажал ли юзер старые инлайновые кнопки со старых бесед с ботом */
            pgdbIssue *getClickedIssue = GetSelectedIssueID(query->message->messageId, uD->pgdbIssues, uD->recentMsgIDs, uD->recentMsgs);

            /* Запросили ключ для разблокировки Revit */
            if (query->data == "req_master_key" && uD->clickedMsgId == query->message->messageId)  /* Старые инлайновые нажатия !clickedMsgId не обрабатываем */
            {
                /* Clear the struct */
                uD->jiraTask = {};
                m_tgBot.getApi().sendMessage(query->message->chat->id, "Введите причину");
                uD->jiraTask.STAT = REVIT_MASTER_KEY_REASON_STEP1;
            }

            if (query->data == "msg_as_bot")
            {
                m_tgBot.getApi().sendMessage(query->message->chat->id, "Пришлите Telegram ID (тип long) получателя, ПРОБЕЛ после ID, и текст для отправки после упомянутого пробела");
                uD->jiraTask.type = "DEVELOPER";
                uD->jiraTask.STAT = DEVELOPER_SEND_MSG_AS_BOT;
            }

            if (query->data == "msg_to_gpt")
            {
                m_tgBot.getApi().sendMessage(query->message->chat->id, "Для выхода из режима GTP-4o нажмёте на команду /start");
                m_tgBot.getApi().sendMessage(query->message->chat->id, "Пришлите текст для отправки GPT (Generative pre-trained transformer)");
                uD->jiraTask.type = "DEVELOPER";
                uD->jiraTask.STAT = DEVELOPER_SEND_MSG_TO_GPT;
            }

            if (query->data == "run_command")
            {
                m_tgBot.getApi().sendMessage(query->message->chat->id, "Пришлите команду для выполнения на хосте");
                uD->jiraTask.type = "DEVELOPER";
                uD->jiraTask.STAT = DEVELOPER_RUN_HOST_COMMAND;
            }

            if (query->data == "reboot_host")
            {
                m_tgBot.getApi().sendMessage(query->message->chat->id, "Перезагружаем компьютер...");
                auto ps = new QProcess();
                ps->start(QString::fromStdString(chanservDirPath) + "/reboot.sh");
                /* Компьютер ушёл на перезагрузку */
            }

            if (query->data == "add_new_contractor" && uD->clickedMsgId == query->message->messageId)  /* Старые инлайновые нажатия !clickedMsgId не обрабатываем */
            {
                m_tgBot.getApi().sendMessage(query->message->chat->id,
                    "Введите данные исполнителя в формате\ntelegram-id\\telegram-link\\Trello-список-id\\E-mail\\ФИО исполнителя\nнапример\n 275911689\\AvgTikhvin\\68033c03d907d7047b2b8f04\\oleg@alabuga.ru\\Олег Булатов");
                uD->jiraTask.type = "STAFF";
                uD->jiraTask.STAT = STAFF_ADD_NEW_CONTRACTOR;
            }

            if (query->data == "set_new_msi_version" && uD->clickedMsgId == query->message->messageId)  /* Старые инлайновые нажатия !clickedMsgId не обрабатываем */
            {
                m_tgBot.getApi().sendMessage(query->message->chat->id, "Пришлите текст в формате X.X.XXX.25, например, 5.4.168.25");
                uD->jiraTask.type = "STAFF";
                uD->jiraTask.STAT = STAFF_SET_NEW_MSI_VERSION;
            }

            if (query->data == "send_file")  /* старые инлайновые нажатия !clickedMsgId не обрабатываем */
            {
                inThread SendDocumentAll([this] { SendDocumentToAll(m_tgBot.getApi(), Consumers, "https://www.oexxxxxxxxxxxxx.ru/instruksiya_xxxx_xxx_helper.pdf",
                                  "Здравствуйте!\n\nНаправляем инструкцию по использованию бота BIM Helper.\n\nС уважением,\nBIM-отдел"); });
            }

            if (query->data == "no_more_photos" && uD->jiraTask.STAT == SHARED_FIRSTNAME_STEP5)           /* Нет больше фото */
            {
                c_out << "Callback от " << itoa(query->message->chat->id) << " \"Нет больше фото\"" << std::endl;
                m_tgBot.getApi().sendMessage(query->message->chat->id, "Отлично\nНапишите свои имя и фамилию", nullptr, nullptr, kbGanzel);
                uD->jiraTask.STAT = SHARED_SEPARATE_TEKLA_FROM_BIM_STEP6;
            }

            /* МОИ ЗАДАЧИ ****************************************************************** / kbInlineContractor / *****************/

            if (query->data == "my_tasks" && uD->clickedMsgId == query->message->messageId)  /* старые инлайновые нажатия !clickedMsgId не обрабатываем */
            {
                uD->jiraTask = {};          /* Clear the struct  */
                uD->clickedMsgId = 0;
                inThread GetIssues([this, query]{ GetMyIssues(query->from->id); });
            };


            /* TEKLA ****************************************************************** / kbInlineTekla / *****************/

            if (query->data == "tekla_enter_menu" && uD->clickedMsgId == query->message->messageId)  /* старые инлайновые нажатия !clickedMsgId не обрабатываем */
            {
                /* Изменяем текст кнопки "Текла" */
                m_tgBot.getApi().editMessageReplyMarkup(query->message->chat->id, query->message->messageId, "", kbTekla);
                uD->jiraTask.subj = "Tekla";
            }

            if ((query->data == "tekla_general_issue"))
            {
                /* Убираем "Текла"-кнопки  */
                m_tgBot.getApi().deleteMessage(query->message->chat->id, query->message->messageId);

                m_tgBot.getApi().sendMessage(query->message->chat->id, "Напишите одним словом тему обращения");
                uD->jiraTask.type = "TEKLA";
                uD->jiraTask.cardCaption = uD->jiraTask.section = "Проблемы";
                uD->jiraTask.STAT = TASK_SUBJ_STEP3;
            }

            if ((query->data == "tekla_model_repair"))
            {
                if (!chatSessions->hasSession(query->from->id)) {
                    c_out << "Creating new chat session for tg user " << itoa(query->from->id) << std::endl;
                    chatSessions->createNewTgSession(query->from->id);
                }
                chatSessions->passMessage(query->from->id, query->message->text);

                /* Убираем "Текла"-кнопки  */
                m_tgBot.getApi().deleteMessage(query->message->chat->id, query->message->messageId);

                m_tgBot.getApi().sendMessage(query->message->chat->id, "Впишите путь модели:");
                uD->jiraTask.type = "TEKLA";
                uD->jiraTask.cardCaption = uD->jiraTask.section = "Поломка модели";
                uD->jiraTask.STAT = TEKLA_REPAIR_MODEL_STEP1;
            }

            if (query->data == "tekla_back" && uD->clickedMsgId == query->message->messageId)  /* старые инлайновые нажатия !clickedMsgId не обрабатываем */
            {
                /* Возвращаемся в главное меню кнопок */
                m_tgBot.getApi().editMessageReplyMarkup(query->message->chat->id, query->message->messageId, "", kbConsumer);
                uD->jiraTask = {};          /* Clear the struct  */
            }

            /* ПРИСТУПИТЬ К ВЫПОЛНЕНИЮ *********************************************************************************************************************/

            if ( (query->data == "start_solving") && getClickedIssue != NULL            /* kbInlineStartSolving */
                    && uD->clickedMsgId == 0)       /* Эта проверка добавлена чтобы не срабатывали многкратные нажатия на кнопку "Приступить к выполнению", т.к. выбросывается TgException */
                         /* Bad Request: message is not modified: specified new message content and reply markup are exactly the same as a current content and reply markup of the message */
            {
                /* Изменяем текст кнопки, прикрепленной к задаче - с "Приступить" на "Завершить" */
                try
                {
                    m_tgBot.getApi().editMessageReplyMarkup(query->message->chat->id, query->message->messageId, "", kbTaskClosing);
                    uD->clickedMsgId = query->message->messageId;
                } catch (TgException e) { }

                /* MessageBox: */
                m_tgBot.getApi().answerCallbackQuery(query->id, "Вы приступили к выполнению задачи.\nПосле завершения задачи, задача будет недоступна, и отправится в архив", true);

                /* Изменяем цвет карточки в Trello */
                m_trelloAPI->ChangeCardColor(getClickedIssue->idCard, "green");
            }

            /* НОВАЯ ЗАДАЧА BIM *********************************************************************************************************************/

            if(query->data == "new_task")
            {
                uD = uniqData(query->from->id);
                if (uD)
                {
                    if (uD->clickedMsgId != query->message->messageId)  /* старые инлайновые нажатия !clickedMsgId не обрабатываем */
                        return;
                    uD->clickedMsgId = 0;

                    uD->jiraTask = {};          /* Clear the struct  */
                    /* Обрабатываем запросы - только, если это наш сотрудник */
/*                  if (isAllowedConsumer(query->from->id) || isAllowedContractor(query->from->id))                                 */
                    {
                        c_out << query->message->chat->username + " начал заполнять заявку на новую задачу." << std::endl;
                        uD->jiraTask.type = "BIMTASK";
                        OnNewTaskQuery(query);
                    }
                }
            }

            /* СЕМЕЙСТВА **************************************************************************************************************************/

            if(query->data == "new_family")
            {
                uD = uniqData(query->from->id);
                if (uD)
                {
                    if (uD->clickedMsgId != query->message->messageId)  /* старые инлайновые нажатия !clickedMsgId не обрабатываем */
                        return;
                    uD->clickedMsgId = 0;

                    uD->jiraTask = {};          /* Clear the struct  */
                    /* Обрабатываем запросы - только, если это наш сотрудник */
/*                  if (isAllowedConsumer(query->from->id) || isAllowedContractor(query->from->id))                                 */
                    {
                        c_out << query->message->chat->username + " начал заполнять заявку на новое семейство." << std::endl;
                        uD->jiraTask.subj = "Создать семейство";
                        uD->jiraTask.type = "FAMILY";
                        OnNewFamilyQuery(query);
                    }
                }
            }

            if ( (query->data == "more_photos") && getClickedIssue != NULL)           /* kbInlineStartSolvingWPhotos */
            {
                OnMorePhotosCommand(*getClickedIssue, query);
            }


            /* === TASK_CLOSING ======================================================================================================================= */

            /* Чтобы не получить бан "429 too many requests", фильтруем - не нажал ли юзер старые инлайновые кнопки со старых бесед с ботом */
            if ((query->data == "task_closing") && getClickedIssue != NULL &&             /* kbInlineTaskClosing */
                    uD->clickedMsgId != 0)       /* Эта проверка добавлена чтобы не срабатывали многкратные нажатия на кнопку "Завершить выполнение", т.к. выбросывается TgException */
            /* Bad Request: message is not modified: specified new message content and reply markup are exactly the same as a current content and reply markup of the message */
            {
                c_out << itoa(uD->clickedMsgId) << " TC: MsgID " << itoa(query->message->messageId) << " QueryID " << query->id << std::endl;
                /* Изменяем текст кнопки, прикрепленной к задаче - с "Завершить" на "Выполнена" */
                m_tgBot.getApi().editMessageReplyMarkup(query->message->chat->id, query->message->messageId, "", kbTaskDone);
                uD->clickedMsgId = 0;
                /* MessageBox: */
                m_tgBot.getApi().answerCallbackQuery(query->id, "Вы завершили задачу\nМожно приступать к выполнению следующей", true);

                /* Уведомление о выполнении задачи для админ.аппарата */
                inThread notifyStaff([this, getClickedIssue]{ SendMsgToStaff(m_tgBot.getApi(), Staff,  "Исполнитель " + getClickedIssue->ContractorName + " @" + getClickedIssue->ContractorLink + \
                                        "\nвыполнил задачу " +
                                        itoa(getClickedIssue->id) + " " +
                                        getClickedIssue->issueNum + " " +
                                        getClickedIssue->Theme + " от пользователя\n" + getClickedIssue->From, kbGoDashboard); });

                /* Архивируем карточку на доске Trello */
                m_trelloAPI->ArchivingCard(getClickedIssue->idCard);

                /* И удаляем карточку из нашей БД */
                if (DeleteIssueFromDB(&taskDB, QString::number(getClickedIssue->id)))
                {
                    std::cout << "Карточка " << getClickedIssue->id << " удалена из БД" << std::endl;
                    DeleteIssueFromDB(&pgDatabase, QString::number(getClickedIssue->id));
                }
            }
        }
    });
}

void ChanservBot::OnNewTaskQuery(CallbackQuery::Ptr query)
{
    uD = uniqData(query->from->id);
    if (uD)
    {
        m_tgBot.getApi().sendMessage(query->message->chat->id, "Напишите свой раздел", nullptr, nullptr, kbGanzel);
        uD->jiraTask.STAT = TASK_SECTION_STEP1;
    }
}

/* Creation new Revit family */
void ChanservBot::OnNewFamilyQuery(CallbackQuery::Ptr query)
{
    uD = uniqData(query->from->id);
    if (uD)
    {
        m_tgBot.getApi().sendMessage(query->message->chat->id, "<u><b>Раздел</b></u> семейства:",
                                   nullptr, nullptr, kbGanzel, "HTML");
        uD->jiraTask.STAT = FAMILY_UNIT_STEP1;
    }
}

