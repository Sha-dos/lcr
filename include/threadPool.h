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

    struct ThreadInfo {
        int id;
        std::atomic<bool> active{false};
        std::atomic<int> taskId{-1};
    };

    std::vector<ThreadInfo> threadInfo;

public:
    ThreadPool(size_t threads) {
        threadInfo.resize(threads);

        for(size_t i = 0; i < threads; ++i) {
            threadInfo[i].id = i;

            workers.emplace_back([this, i] {
                while(true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        threadInfo[i].active = false;
                        threadInfo[i].taskId = -1;

                        condition.wait(lock, [this] {
                            return stop || !tasks.empty();
                        });

                        if(stop && tasks.empty()) {
                            return;
                        }

                        task = std::move(tasks.front());
                        tasks.pop();
                    }

                    threadInfo[i].active = true;
                    threadInfo[i].taskId = active_tasks.fetch_add(1);

                    task();

                    active_tasks--;
                }
            });
        }
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

    const std::vector<ThreadInfo>& getThreadStatus() const {
        return threadInfo;
    }
};

#endif //LCR_THREADPOOL_H
