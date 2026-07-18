#include "file_indexer/ThreadPool.hpp"

#include <stdexcept>
#include <utility>

namespace file_indexer {

ThreadPool::ThreadPool(const std::size_t workerCount) {
    if (workerCount == 0) {
        throw std::invalid_argument("worker count must be greater than zero");
    }

    workers_.reserve(workerCount);
    try {
        for (std::size_t index = 0; index < workerCount; ++index) {
            workers_.emplace_back(&ThreadPool::workerLoop, this);
        }
    } catch (...) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
        }
        taskAvailable_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        throw;
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    taskAvailable_.notify_all();
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::submit(std::function<void()> task) {
    if (!task) {
        throw std::invalid_argument("cannot submit an empty task");
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) {
            throw std::runtime_error("cannot submit work to a stopping thread pool");
        }
        tasks_.push(std::move(task));
    }
    taskAvailable_.notify_one();
}

void ThreadPool::waitUntilIdle() {
    std::unique_lock<std::mutex> lock(mutex_);
    idle_.wait(lock, [this] { return tasks_.empty() && activeTasks_ == 0; });
}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            taskAvailable_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });
            if (stopping_ && tasks_.empty()) {
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
            ++activeTasks_;
        }

        try {
            task();
        } catch (...) {
            // A task failure must not terminate a worker. Callers that need error
            // details should catch and record them inside the submitted task.
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            --activeTasks_;
            if (tasks_.empty() && activeTasks_ == 0) {
                idle_.notify_all();
            }
        }
    }
}

}  // namespace file_indexer
