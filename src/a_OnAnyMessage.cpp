#include "chanserv.h"
#include "support.h"

#include <boost/coroutine2/coroutine.hpp>

using namespace TgBot;

void ChanservBot::SessionDispatcher(){
m_tgBot.getEvents().onAnyMessage([this](Message::Ptr message)
{
    if (!isDBConsumer(message->from->id))
    {
        c_out << "Клиент " + message->from->username + " (" + itoa(message->from->id) + ") отсутствует в базе, добавляем в его таблицу dbo.consumers" << std::endl;
/*      AddConsumerToDB(&msDatabase, message->from->id, message->from->username);   */
        AddConsumerToDB(&pgDatabase, message->from->id, message->from->username);
        ReloadConsumersList();
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

    if (std::find(allowedContractors.begin(), allowedContractors.end(), message->from->id) != allowedContractors.end()) std::cout << "исполнителя ";
/*  if (std::find(allowedConsumers.begin(), allowedConsumers.end(), itoa(message->from->id)) != allowedConsumers.end()) std::cout << " заказчика ";     */

    std::cout << message->from->firstName << " @" << message->from->username << " (" << message->from->id << "), текст: \"" << message->text << "\"" << std::endl;

    /* Если на нового написавшего еще не заведено дело, то добавляем его в массив юзеров, активных в данном запуске бота */
    uD = uniqData(message->from->id);
    if (!uD)
    {
        uD = new userDispatcher(message->from->id, message->from->username, message->from->firstName);
        /* uD->jiraTask = new newTask(); */
        uDv.push_back(uD);
    }


    /* ==== Тестовый блок личного обращения к боту, тестовые чаты ======================================================== */

    if (message->replyToMessage != nullptr)
        if (message->replyToMessage->from->id == 7399766521)
        {
            if (message->chat->id == -1002041703096 || message->chat->id == -4944647950)
            {
                inThread SendMsgToGpt([this, message] { MsgToDeepTransformer(chatgpt_url, m_tgBot.getApi(), message->chat->id, nullptr, message->text); });
            }
        }

    if ((message->chat->id == -1002041703096 || message->chat->id == -4944647950) && message->text.rfind("@bim_alde_bot", 0) == 0) /* string starts with */
    {
        inThread SendMsgToGpt([this, message] { MsgToDeepTransformer(chatgpt_url, m_tgBot.getApi(), message->chat->id, nullptr, message->text); });
    }
    /* =================================================================================================================== */

});                                                         };

void ChanservBot::HandleSmallTalk()
{
    m_tgBot.getEvents().onNonCommandMessage([this](Message::Ptr message)
    {
        uD = uniqData(message->from->id);
        if (uD)
        {
          switch (uD->jiraTask.STAT) {
            case REVIT_MASTER_KEY_FIRSTNAME_STEP2: /* Прислали ФИО для ключа Revit */
            {
                message->text = truncate_msg_from_user(message->text, 3000);
                uD->jiraTask.ConsumerName = message->text;

                InlineKeyboardMarkup::Ptr kbRejectOrApprove = std::shared_ptr<InlineKeyboardMarkup>(new InlineKeyboardMarkup);
                std::vector<InlineKeyboardButton::Ptr> row_key_request;
                InlineKeyboardButton::Ptr btnApproveRequest = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
                btnApproveRequest->text = "Одобрить";
                std::string approveCallbackData = "approve_key_request_" + itoa(message->from->id);
                btnApproveRequest->callbackData = approveCallbackData;

                row_key_request.push_back(btnApproveRequest);

                InlineKeyboardButton::Ptr btnRejectRequest = std::shared_ptr<InlineKeyboardButton>(new InlineKeyboardButton);
                btnRejectRequest->text = "Отказать";
                std::string rejectCallbackData = "_reject_key_request_" + itoa(message->from->id);
                btnRejectRequest->callbackData = rejectCallbackData;
                row_key_request.push_back(btnRejectRequest);

                kbRejectOrApprove->inlineKeyboard.push_back(row_key_request);

                inThread dialogWithStaff([this, message, kbRejectOrApprove]{ SendMsgToStaff(m_tgBot.getApi(), Staff, "Запрос одобрения ключа от " +
                            uD->jiraTask.ConsumerName +
                            // message->from->firstName + " " + message->from->lastName + " (" + message->from->username + ", " + itoa(message->from->id) + ")" +
                            " Причина: " + uD->jiraTask.keyReason, kbRejectOrApprove); });

                m_tgBot.getApi().sendMessage(message->chat->id, "Ожидайте одобрения");

                /* Чистим статус текущего этапа общения с ботом, иначе 'Ожидайте одобрения' данный блок case будет постоянно повторяться */
                uD->jiraTask.STAT = 0;
                break;
            }
            case REVIT_MASTER_KEY_REASON_STEP1: /* Прислали причину запроса ключа для Revit */
            {
                message->text = truncate_msg_from_user(message->text, 3000);
                uD->jiraTask.keyReason = message->text;
                m_tgBot.getApi().sendMessage(message->from->id, "Напишите свои имя и фамилию");
                uD->jiraTask.STAT = REVIT_MASTER_KEY_FIRSTNAME_STEP2;
                break;
            }
            case STAFF_ADD_NEW_CONTRACTOR:
            {
                QStringList contractorParts = QString::fromStdString(message->text).split("\\");
                /* Добавляем нового исполнителя в БД в таблицу contractors */
                const QString insContractorsTable = "INSERT INTO contractors (id, name, link, trello_list, email) VALUES ('" + contractorParts.at(0) + "'," +
                                              "'" + contractorParts.at(4) + "'," +
                                              "'" + contractorParts.at(1) + "'," +
                                              "'" + contractorParts.at(2) + "'," +
                                              "'" + contractorParts.at(3) + "');";

                QSqlQuery pgQuery(pgDatabase);
                c_out << "Adding new contractor into DB from line " + message->text << std::endl;
                pgQuery.exec(insContractorsTable);
                pgQuery.next();
                c_out << "pgSqlexec: " << pgQuery.lastError().text().toStdString() << std::endl;

                m_tgBot.getApi().sendMessage(message->chat->id, "Сотрудник " + contractorParts.at(1).toStdString() + " добвавлен в качестве исполнителя");

                ReloadContractorsList();
                uD->jiraTask  = {};  /* Очистили структуру */
                break;
            }

            case DEVELOPER_SEND_MSG_TO_GPT:
            {
                inThread SendMsgToGpt([this, message] { MsgToDeepTransformer(chatgpt_url, m_tgBot.getApi(), message->from->id, uD, message->text); });
                break;
            }

            case DEVELOPER_SEND_MSG_AS_BOT:
            {
                long receiverTgID = atol(message->text.substr(0, message->text.find(' ')).c_str());
                std::string textToSend = message->text.substr(message->text.find(" ") + 1);
                try {
                    m_tgBot.getApi().sendMessage(receiverTgID, textToSend);
                } catch (TgException e) {
                    m_tgBot.getApi().sendMessage(message->chat->id, "try/catch: Сообщение " + textToSend + " телеграм юзеру " + itoa(receiverTgID) + " НЕ отправлено");
                }

                m_tgBot.getApi().sendMessage(message->chat->id, "Сообщение " + textToSend + " телеграм юзеру " + itoa(receiverTgID) + " отправлено");

                uD->jiraTask  = {};  /* Очистили структуру */
                break;
            }

            /* Запуск системной команды на хост-компьютере */
            case DEVELOPER_RUN_HOST_COMMAND:
            {
                std::string readFromCLI;
                auto ps = new QProcess();

                if (QString::fromStdString(message->text).contains(QChar::Space)) /* Есть аргументы */
                {
                    /* Первое слово выносим в отдельную переменную */
                    std::string selfTheCommand = message->text.substr(0, message->text.find(" "));
                    std::string allParameters = message->text.substr(message->text.find_first_of(" \t")+1);

                    QStringList argumentsList = QProcess::splitCommand(QString::fromStdString(allParameters));
                    ps->start(QString::fromStdString(selfTheCommand), argumentsList);
                }
                else
                    ps->start(QString::fromStdString(message->text));

                if (ps->waitForStarted(-1)) {
                    while(ps->waitForReadyRead(-1)) {
                        readFromCLI = ps->readAllStandardOutput();
                    }
                    SendMsgToSupprt(m_tgBot.getApi(), sett["support_TgID"], readFromCLI, kbDeveloper);
                }
                /* Ошибка */
                else
                {
                    readFromCLI = ps->readAllStandardOutput();
                    QString p_stderr = ps->readAllStandardError();
                    SendMsgToSupprt(m_tgBot.getApi(), sett["support_TgID"], "Ошибка выполнения команды " + p_stderr.toStdString(), kbDeveloper);
                }

                delete ps;
                break;
            }

            case STAFF_SET_NEW_MSI_VERSION:
            {
                /* Записываем новую версию .msi в БД */
                const QString updSettingsTable = "UPDATE settings SET value = '" + QString::fromStdString(message->text) + "' WHERE name = 'MSI_actual_version'";
                QSqlQuery Qt_Query(msDB);
                c_out << "Updating .msi version to: " + message->text << std::endl;
                auto r_ret = Qt_Query.exec(updSettingsTable);

                m_tgBot.getApi().sendMessage(message->chat->id, "Версия сборки плагина актуализирована в БД до " + message->text);

                uD->jiraTask  = {};  /* Очистили структуру */
                break;
            }            

            case SHARED_SEPARATE_TEKLA_FROM_BIM_STEP6:
            {
                if (message->text != "") /* Прислали текст, не аудио-видео etc */
                {
                    /* Прислали имя заказчика */
                    message->text = truncate_msg_from_user(message->text, 150);
                    uD->jiraTask.ConsumerName = message->text;

                    uD->jiraTask.ConsumerTgId = message->chat->id;
                    uD->jiraTask.ConsumerTgUsernameLink = message->chat->username;
                    uD->jiraTask.STAT = SHARED_FINAL_STEP7;
                    /* без case break - переходим сразу к следующему case */
                }
                else break;
            }
            case SHARED_FINAL_STEP7: /* Пора создавать карточку   */
            {
                inThread createIssue([this, message]{ CreateNewIssue(message, uD); });
                break;
            }

            case TASK_DESCR_STEP4: /* Нам прислали описание */
            {
                message->text = truncate_msg_from_user(message->text, 3000);
                uD->jiraTask.description = message->text;
                coutSmallTalk(*uD, "Description", message);
                m_tgBot.getApi().sendMessage(message->chat->id, "Пришлите изображения и <i><b>после их отправки</b></i> нажмите кнопку \"Нет больше фото\"",
                                          nullptr, nullptr, kbManagePhotos, "Html");
                uD->jiraTask.STAT = SHARED_FIRSTNAME_STEP5;
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

            case TASK_SUBJ_STEP3: /* Нам прислали тему */
            {
                message->text = std::regex_replace(message->text, std::regex("[\r\t\n]+" ), "");
                message->text = truncate_msg_from_user(message->text, 150);
                uD->jiraTask.subj = message->text;

                coutSmallTalk(*uD, "Subject", message);
                m_tgBot.getApi().sendMessage(message->chat->id, "Опишите максимально подробно проблему");
                uD->jiraTask.STAT = TASK_DESCR_STEP4;
                break;
            }

            case TASK_PROJECT_STEP2: /*  Нам прислали название проекта */
            {
                message->text = std::regex_replace(message->text, std::regex("[\r\t\n]+" ), "");
                message->text = truncate_msg_from_user(message->text, 150);
                uD->jiraTask.project = message->text;
                coutSmallTalk(*uD, "Project", message);
                m_tgBot.getApi().sendMessage(message->chat->id, "Записано\nНапишите одним словом тему обращения");
                uD->jiraTask.STAT = TASK_SUBJ_STEP3;
                break;
            }

           case TASK_SECTION_STEP1: /* Нам прислали название раздела    */
           {
               message->text = std::regex_replace(message->text, std::regex("[\r\t\n]+" ), "");
               message->text = truncate_msg_from_user(message->text, 150);
                uD->jiraTask.section = message->text;

                coutSmallTalk(*uD, "Section", message);
                m_tgBot.getApi().sendMessage(message->chat->id, "Записано\nНапишите, по какому проекту вопрос?\nНапример: Синергия 11.1.3 или Кампусы ВН");
                uD->jiraTask.STAT = TASK_PROJECT_STEP2;
                break;
            }

            case SHARED_FIRSTNAME_STEP5:
            if (message->text.empty()) /* Нам прислали фото */
            {
                inThread receivePhoto([this, message]{ ReceivePhotos(message, uD); });
                break;
            }

            case FAMILY_PROJECT_STEP4: /* Нам прислали путь (поле project в БД) */
            {
                message->text = std::regex_replace(message->text, std::regex("[\r\t\n]+" ), "");
                message->text = truncate_msg_from_user(message->text, 150);
                uD->jiraTask.project = message->text;
                coutSmallTalk(*uD, "Path / proj:", message);
                m_tgBot.getApi().sendMessage(message->chat->id, "Пришлите изображения и <b>после</b> их отправки нажмите кнопку \"Нет больше фото\"",
                                          nullptr, nullptr, kbManagePhotos, "Html");
                uD->jiraTask.STAT = SHARED_FIRSTNAME_STEP5;
                break;
            }

            case  FAMILY_DESCR_STEP3: /* Нам прислали описание */
            {
               message->text = truncate_msg_from_user(message->text, 3000);
               uD->jiraTask.description = message->text;
               coutSmallTalk(*uD, "Description", message);
               m_tgBot.getApi().sendMessage(message->chat->id, "<u><b>Путь/ссылка</b></u> на документацию (сайт или семейство):", nullptr, nullptr, nullptr, "Html");
               uD->jiraTask.STAT = FAMILY_PROJECT_STEP4;
               break;
            }

            case FAMILY_CATEGORY_STEP2: /*   Нам прислали категорию  */
            {
                message->text = std::regex_replace(message->text, std::regex("[\r\t\n]+" ), "");
                message->text = truncate_msg_from_user(message->text, 150);
                uD->jiraTask.category = message->text;
                coutSmallTalk(*uD, "Category", message);
                m_tgBot.getApi().sendMessage(message->chat->id, "<u><b>Напиши, что нужно сделать</b></u>:\nЖелательно указать:\n -параметры;\n -информацию об УГО", nullptr, nullptr, nullptr, "Html");
                uD->jiraTask.STAT = FAMILY_DESCR_STEP3;
                break;
            }

            case FAMILY_UNIT_STEP1: /* Нам прислали раздел */
            {
                message->text = std::regex_replace(message->text, std::regex("[\r\t\n]+" ), "");
                message->text = truncate_msg_from_user(message->text, 150);

                uD->jiraTask.section = message->text;
                coutSmallTalk(*uD, "Section", message);
                m_tgBot.getApi().sendMessage(message->chat->id, "Записано\n<u><b>Категория</b></u> семейства: ", nullptr, nullptr, nullptr, "Html");
                uD->jiraTask.STAT = FAMILY_CATEGORY_STEP2;
                break;
            }

            default:
               break;
            }
        }
    });
}

