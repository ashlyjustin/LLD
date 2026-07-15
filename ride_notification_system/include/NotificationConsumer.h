#pragma once
#include <thread>
#include <vector>
#include <map>
#include <memory>
#include <iostream>
#include "NotificationQueue.h"
#include "INotificationChannel.h"
#include "channels/PushChannel.h"
#include "channels/SMSChannel.h"
#include "channels/EmailChannel.h"

/*
 * NotificationConsumer — thread pool that drains NotificationQueue.
 *
 * Ownership model:
 *   - Owns the NotificationQueue (composition). Publishers call enqueue(), never
 *     touch the queue directly.
 *   - Each worker thread owns its own channel instances → zero shared mutable
 *     state between workers.
 *
 * Ordering note:
 *   With numWorkers == 1 (default), event ordering per ride is preserved.
 *   With numWorkers > 1, tasks for the same ride may be processed out of order
 *   if workers run at different speeds. Use > 1 only when strict ordering is
 *   not required (e.g. independent notification types).
 *
 * Shutdown (graceful drain):
 *   stop() → queue.stop() → workers drain remaining tasks → threads join.
 *   Destructor calls stop() automatically (RAII).
 */
class NotificationConsumer {
    NotificationQueue        queue;
    std::vector<std::thread> workers;
    int                      numWorkers;
    bool                     started = false;
    std::mutex               printMtx;   // serialises stdout across threads

    // Each worker thread builds its own channel map — no shared state.
    static std::map<ChannelType, std::unique_ptr<INotificationChannel>> makeChannels() {
        std::map<ChannelType, std::unique_ptr<INotificationChannel>> ch;
        ch[ChannelType::PUSH]  = std::make_unique<PushChannel>();
        ch[ChannelType::SMS]   = std::make_unique<SMSChannel>();
        ch[ChannelType::EMAIL] = std::make_unique<EmailChannel>();
        return ch;
    }

    void work(int workerId) {
        auto channels = makeChannels();

        while (auto task = queue.pop()) {
            for (ChannelType ch : task->recipient->getPreferredChannels()) {
                auto it = channels.find(ch);
                if (it == channels.end()) continue;
                try {
                    std::lock_guard<std::mutex> lock(printMtx);
                    std::cout << "  [Worker-" << workerId << "] ";
                    it->second->send(*task->recipient, task->notification);
                } catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lock(printMtx);
                    std::cerr << "  [Worker-" << workerId << "][WARN] "
                              << toString(ch) << " failed: " << e.what() << "\n";
                }
            }
        }

        std::lock_guard<std::mutex> lock(printMtx);
        std::cout << "  [Worker-" << workerId << "] drained and stopped.\n";
    }

public:
    explicit NotificationConsumer(int numWorkers = 1)
        : numWorkers(numWorkers) {}

    // Non-copyable, non-movable (owns threads and a mutex).
    NotificationConsumer(const NotificationConsumer&)            = delete;
    NotificationConsumer& operator=(const NotificationConsumer&) = delete;

    // Enqueue a task for async delivery.
    // Returns false if the consumer has already been stopped (task is dropped).
    bool enqueue(NotificationTask task) {
        return queue.push(std::move(task));
    }

    void start() {
        if (started) return;
        started = true;
        for (int i = 0; i < numWorkers; ++i)
            workers.emplace_back(&NotificationConsumer::work, this, i + 1);
    }

    // Graceful drain: existing tasks are fully processed before threads exit.
    void stop() {
        queue.stop();
        for (auto& t : workers)
            if (t.joinable()) t.join();
    }

    ~NotificationConsumer() { stop(); }
};
