#pragma once

#include <map>
#include <string>
#include <vector>
#include <deque>


#define HEADER_CONTENT_TYPE_APP_JSON "Content-Type: application/json; charset=UTF-8"

constexpr int jira_in_progress = 4;

constexpr int table_bim_issues = 1;
constexpr int table_tekla_issues = 2;

constexpr int tekla_improve_suggestion_step1 = 101;
constexpr int tekla_repair_model_step1 = 102;
constexpr int tekla_general_issue_step1 = 103;

constexpr int developer_send_msg_as_bot  = 201;
constexpr int developer_send_msg_to_gpt  = 202;
constexpr int developer_run_host_command = 203;

constexpr int staff_set_new_msi_version  = 301;
constexpr int staff_add_new_contractor  = 302;

constexpr int task_section_step1  = 401;
constexpr int task_project_step2 = 402;
constexpr int task_subj_step3 = 403;
constexpr int task_descr_step4 = 404;

constexpr int family_unit_step1 = 501;
constexpr int family_category_step2 = 502;
constexpr int family_descr_step3 = 503;
constexpr int family_project_step4 = 504;

constexpr int shared_firstname_step5 = 605;
constexpr int shared_separate_tekla_from_bim_step6 = 606;
constexpr int shared_final_step7 = 607;

constexpr int revit_master_key_reason_step1 = 701;
constexpr int revit_master_key_firstname_step2 = 702;

struct new_task
{
    std::string tekla_card_caption;
    std::string section;
    std::string project;
    std::string subj;
    std::string category;
    std::string description;

 /* BIMTASK, FAMILY, MASTERKEY or TEKLA?  */
    std::string type;

    long consumer_tg_id;
    std::string consumer_tg_username_link;
    std::string consumer_name;
    int stat_;
    std::string id_card;
    std::string creation_date;
    std::vector<std::string> v_docu_ids;
    std::string photoIDs;
    std::vector<std::string> v_local_file_paths;
    /* Причина запроса мастер-ключа*/
    std::string key_reason;
};

struct ms_pg_db
{
    std::string server;
    std::string database;
    std::string port;
    std::string user;
    std::string password;
};

struct most_recent_msgs
{
    int ms_query_row_id;
    int msg_id;
};

struct contractor
{
    long id;
    std::string name;
    std::string link;
    std::string trello_list;
};

struct pgdb_issue
{
    int id;
    int sent_msg_id; /* этот элемент msdbIssue был отправлен в чат (в блоке ответов "Мои задачи"),
                        затем id отправленного сообщения сохранён прямо в этот int sentMsgID,
                        для цитирования (ссылаться) когда запросят больше картинок командой more    */
    long sent_chat_id;

    std::string id_card;
    std::string issue_num;
    std::string unit;
    std::string theme;
    std::string project;
    std::vector<std::string> photos;
    std::string from;
    std::string category;
    std::string description;
    std::string creation_date;
    std::string contractor_list;
    std::string contractor_name;
    std::string contractor_link;    
};

struct user_dispatcher
{
    long chat_user_tg_id;
    std::string chat_user_tg_name;
    std::string chat_user_first_name;

    std::string *created_card_id = nullptr;
    std::string *created_jira_task_id = nullptr;
    new_task jira_task;
    int last_error = 0;
    /* std::map<long, newTask*> jiraTask;   */
    std::vector<pgdb_issue> pgdb_issues;
    std::vector<most_recent_msgs> recent_msgs;
    int clicked_msg_id;
    std::vector<int> recent_msg_ids;
};
