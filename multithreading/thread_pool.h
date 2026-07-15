#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <thread>
#include <queue>
#include <future>
#include <functional>
#include <mutex>
#include <condition_variable>


class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();

    // Submit task and return future
    template <typename Func, typename... Args>
    auto submit(Func &&f, Args &&...args) {
        using return_type = std::invoke_result_t<Func, Args...>;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<Func>(f), std::forward<Args>(args)...)
        );
        auto future = task->get_future();
        {
            std::unique_lock<std::mutex> lock(mutex_);
            tasks_.emplace([task] { (*task)(); });
        }
        cv_.notify_one();
        return future;
    }

    // Submit task and invoke callback with result in main thread (non-blocking for caller)
    template <typename Func, typename Callback, typename... Args>
    void submit_with_callback(Func&& f, Callback&& cb, Args&&... args) {
        using return_type = std::invoke_result_t<Func, Args...>;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<Func>(f), std::forward<Args>(args)...)
        );
        auto main_cb = [task, cb = std::forward<Callback>(cb)]() mutable {
            cb(task->get_future().get());
        };
        {
            std::unique_lock<std::mutex> lock(mutex_);
            tasks_.emplace([task, this, main_cb]() mutable {
                (*task)();
                // After task is done, push callback to main thread queue
                {
                    std::lock_guard<std::mutex> lock(main_cb_mutex_);
                    main_callbacks_.emplace(main_cb);
                }
            });
        }
        cv_.notify_one();
    }

    // Call this from the main/original thread to process callbacks
    void process_callbacks() {
        std::queue<std::function<void()>> local_queue;
        {
            std::lock_guard<std::mutex> lock(main_cb_mutex_);
            std::swap(local_queue, main_callbacks_);
        }
        while (!local_queue.empty()) {
            local_queue.front()();
            local_queue.pop();
        }
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_;

    // For main-thread callbacks
    std::queue<std::function<void()>> main_callbacks_;
    std::mutex main_cb_mutex_;
};

#endif // THREAD_POOL_H
