// =========================================================================
// threadPool.h
// =========================================================================

#ifndef LCR_THREADPOOL_H
#define LCR_THREADPOOL_H

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <functional>

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop{false};
    std::atomic<int> active_tasks{0};

public:
    ThreadPool(size_t threads) {
        for(size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while(true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] {
                            return stop || !tasks.empty();
                        });

                        if(stop && tasks.empty()) {
                            return;
                        }

                        task = std::move(tasks.front());
                        tasks.pop();
                    }

                    active_tasks++;
                    task();
                    active_tasks--;
                }
            });
        }
    }

    template<class F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one();
    }

    int getActiveTasks() const {
        return active_tasks;
    }

    int getQueueSize() {
        std::unique_lock<std::mutex> lock(queue_mutex);
        return tasks.size();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for(std::thread &worker: workers) {
            if(worker.joinable()) {
                worker.join();
            }
        }
    }
};

#endif //LCR_THREADPOOL_H
