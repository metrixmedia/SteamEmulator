/*
 * Copyright (C) Nemirtingas
 * This file is part of System.
 *
 * System is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * System is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the System; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

namespace System {
class ThreadPool
{
    using task_t = std::function<void()>;

    std::atomic<bool> _StopWorkers;
    std::atomic<std::size_t> _ActiveCount;

    std::condition_variable _WorkerNotifier;
    std::mutex _Mutex;

    std::vector<std::thread> _Workers;
    std::queue<task_t> _Tasks;

public:
    explicit ThreadPool():
        _ActiveCount(0)
    {
    }

    ~ThreadPool()
    {
        Join();
    }

    ThreadPool(ThreadPool const &) = delete;
    ThreadPool(ThreadPool&&) = default;

    ThreadPool&operator=(ThreadPool const &) = delete;
    ThreadPool&operator=(ThreadPool&&) = default;

    template <class Func, class... Args>
    auto Push(Func &&fn, Args &&...args)
    {
        using return_type = typename std::result_of<Func(Args...)>::type;

        auto task{ std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<Func>(fn), std::forward<Args>(args)...)
        ) };

        auto future{ task->get_future() };
        {
            std::lock_guard<std::mutex> lock(_Mutex);

            _Tasks.emplace([task]()
            {
                (*task)();
            });
        }
        
        _WorkerNotifier.notify_one();
        return future;
    }

    // Remove all pending tasks from the queue
    void Clear()
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        _Tasks = {};
    }

    // Stops all previous and creates new worker threads.
    void Start(std::size_t worker_count = std::thread::hardware_concurrency())
    {
        Join();

        _StopWorkers = false;
        for (std::size_t i = 0; i < worker_count; ++i)
            _Workers.emplace_back(std::bind(&ThreadPool::_WorkerLoop, this));
    }

    // Wait all workers to finish
    void Join()
    {
        _StopWorkers = true;
        _WorkerNotifier.notify_all();

        for (auto &thread : _Workers)
        {
            if (thread.joinable())
                thread.join();
        }

        _Workers.clear();
    }

    std::size_t WorkerCount() const
    {
        return _Workers.size();
    }

    // Get the number of active workers
    std::size_t ActiveCount() const
    {
        return _ActiveCount;
    }

private:
    void _WorkerLoop()
    {
        while (true)
        {
            auto task{ _NextTask() };

            if (task)
            {
                ++_ActiveCount;
                task();
                --_ActiveCount;
            }
            else if (_StopWorkers)
            {
                break;
            }
        }
    }

    task_t _NextTask()
    {
        std::unique_lock<std::mutex> lock{ _Mutex };

        _WorkerNotifier.wait(lock, [this]() { return !_Tasks.empty() || _StopWorkers; });

        if (_Tasks.empty())
            return {};

        auto task{ _Tasks.front() };
        _Tasks.pop();
        return task;
    }
};
}