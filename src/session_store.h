#pragma once

#include "structures.h"

#include <chrono>
#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>

/* Сессии юзеров, активных в данном запуске бота. Неактивные дольше ttl удаляются через evict_idle() */
class session_store
{
public:
    using clock = std::chrono::steady_clock;

    /* Возвращает сессию и отмечает активность, nullptr если сессии нет */
    user_dispatcher* find(const long tg_id, const clock::time_point now = clock::now())
    {
        const auto it = m_sessions.find(tg_id);
        if (it == m_sessions.end())
            return nullptr;

        it->second->last_activity = now;
        return it->second.get();
    }

    /* Создаёт сессию, если её ещё нет, и возвращает её */
    user_dispatcher* create(const long tg_id, const std::string& tg_name, const std::string& first_name, const clock::time_point now = clock::now())
    {
        if (user_dispatcher* existing = find(tg_id, now))
            return existing;

        auto session = std::make_unique<user_dispatcher>(tg_id, tg_name, first_name);
        session->last_activity = now;
        user_dispatcher* ret = session.get();
        m_sessions.emplace(tg_id, std::move(session));
        return ret;
    }

    /* Удаляет сессии, не активные дольше ttl, возвращает количество удалённых */
    size_t evict_idle(const clock::time_point now, const clock::duration ttl)
    {
        return std::erase_if(m_sessions, [&](const auto& item) { return now - item.second->last_activity > ttl; });
    }

    size_t size() const { return m_sessions.size(); }

private:
    std::unordered_map<long, std::unique_ptr<user_dispatcher>> m_sessions;
};
