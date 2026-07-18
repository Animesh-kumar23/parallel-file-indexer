#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace file_indexer {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t workerCount);
    ~ThreadPool();

    void submit(std::function<void()> task);
    void waitUntilIdle();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

private:
    void workerLoop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable taskAvailable_;
    std::condition_variable idle_;
    std::size_t activeTasks_{};
    bool stopping_{};
};

}  // namespace file_indexer
