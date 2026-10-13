#pragma once

#include "structures.h"

#include <string_view>

/* Роль, необходимая для служебного действия.
 * callback_data присылает клиент, и Telegram не сверяет его с кнопками сообщения,
 * поэтому права проверяются по самому действию, а не по показанной клавиатуре */
enum class required_role { anyone, staff, developer };

inline required_role role_for_callback(std::string_view data)
{
    if (data == "msg_as_bot" || data == "msg_to_gpt" || data == "run_command" || data == "reboot_host" || data == "send_file")
        return required_role::developer;

    if (data == "add_new_contractor" || data == "set_new_msi_version" ||
        data.starts_with("approve_key_request") || data.starts_with("_reject_key_request"))
        return required_role::staff;

    return required_role::anyone;
}

/* Роль для шага диалога (user_dispatcher::jira_task.stat_) */
inline required_role role_for_step(const int stat)
{
    switch (stat) {
    case developer_send_msg_as_bot:
    case developer_send_msg_to_gpt:
    case developer_run_host_command:
        return required_role::developer;
    case staff_set_new_msi_version:
    case staff_add_new_contractor:
        return required_role::staff;
    default:
        return required_role::anyone;
    }
}

/* Разработчик имеет и права staff */
inline bool is_allowed(const required_role role, const bool is_staff, const bool is_developer)
{
    switch (role) {
    case required_role::developer: return is_developer;
    case required_role::staff:     return is_staff || is_developer;
    default:                       return true;
    }
}
