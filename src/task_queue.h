#pragma once

#include <deque>
#include <functional>
#include <mutex>

/* Очередь задач для выполнения в одном (главном) потоке.
 * Другие потоки (вебхуки Jira, мониторинг) только ставят задачи, Telegram API и БД используются только в потоке-исполнителе */
class task_queue
{
public:
    void post(std::function<void()> task)
    {
        std::lock_guard lock(m_mutex);
        m_tasks.push_back(std::move(task));
    }

    /* Выполняет накопленные задачи, возвращает их количество. Задачи, поставленные во время выполнения, ждут следующего вызова */
    size_t run_pending()
    {
        std::deque<std::function<void()>> tasks;
        {
            std::lock_guard lock(m_mutex);
            tasks.swap(m_tasks);
        }
        for (auto& task : tasks)
            task();
        return tasks.size();
    }

    size_t size() const
    {
        std::lock_guard lock(m_mutex);
        return m_tasks.size();
    }

private:
    mutable std::mutex m_mutex;
    std::deque<std::function<void()>> m_tasks;
};
