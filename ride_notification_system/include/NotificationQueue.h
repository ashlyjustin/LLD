#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include "NotificationTask.h"

/*
 * Thread-safe, blocking FIFO queue for NotificationTasks.
 *
 * Push/pop contract:
 *   push() — non-blocking; returns false and drops the task if stop() was called.
 *   pop()  — blocks until a task is available OR the queue is stopped AND drained.
 *            returns nullopt only when stopped==true AND the queue is empty
 *            (signals workers to exit after draining all pending tasks).
 *   stop() — idempotent; wakes all blocked pop() callers to begin graceful drain.
 *
 * Bounded capacity is not enforced (demo). In production, add a capacity check
 * inside push() and block or reject when full to apply backpressure.
 */
class NotificationQueue {
    std::queue<NotificationTask> q;
    mutable std::mutex           mtx;
    std::condition_variable      cv;
    bool                         stopped = false;

public:
    // Enqueue a task. Returns false (and discards task) if already stopped.
    bool push(NotificationTask task) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (stopped) return false;
            q.push(std::move(task));
        }
        cv.notify_one();
        return true;
    }

    // Blocking pop. Returns nullopt only when stopped AND queue is fully drained.
    std::optional<NotificationTask> pop() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !q.empty() || stopped; });
        if (q.empty()) return std::nullopt;   // stopped + drained → signal exit
        auto task = std::move(q.front());
        q.pop();
        return task;
    }

    // Signal all workers to drain remaining tasks and then exit. Idempotent.
    void stop() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (stopped) return;
            stopped = true;
        }
        cv.notify_all();
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return q.size();
    }

    bool isEmpty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return q.empty();
    }
};
