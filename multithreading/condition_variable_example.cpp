#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <chrono>

std::mutex cout_mtx;
void safe_print(const std::string &s) {
    std::lock_guard<std::mutex> lk(cout_mtx);
    std::cout << s << std::endl;
}

// Simple bounded-buffer producer-consumer using condition_variable
int main() {
    safe_print("Condition variable example: starting");

    const size_t buffer_capacity = 5;
    std::queue<int> buffer;
    std::mutex mtx;
    std::condition_variable cv_not_empty;
    std::condition_variable cv_not_full;

    const int total_items = 30;
    const int producer_count = 2;
    const int consumer_count = 2;

    auto producer = [&](int id){
        for (int i = id; i < total_items; i += producer_count) {
            std::unique_lock<std::mutex> lk(mtx);
            cv_not_full.wait(lk, [&]{ return buffer.size() < buffer_capacity; });
            buffer.push(i);
            safe_print("producer " + std::to_string(id) + " pushed " + std::to_string(i));
            lk.unlock();
            cv_not_empty.notify_one();
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
    };

    auto consumer = [&](int id){
        int consumed = 0;
        while (true) {
            std::unique_lock<std::mutex> lk(mtx);
            // wait until not empty or all work done
            cv_not_empty.wait(lk, [&]{ return !buffer.empty(); });
            int val = buffer.front();
            buffer.pop();
            safe_print("consumer " + std::to_string(id) + " popped " + std::to_string(val));
            lk.unlock();
            cv_not_full.notify_one();
            consumed++;
            // simple termination: stop after receiving enough items
            if (val >= total_items - 1 && buffer.empty()) break;
        }
    };

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    for (int i = 0; i < producer_count; ++i) producers.emplace_back(producer, i);
    for (int i = 0; i < consumer_count; ++i) consumers.emplace_back(consumer, i);

    for (auto &p : producers) if (p.joinable()) p.join();

    // After producers done, notify consumers to finish consuming remaining items.
    {
        std::lock_guard<std::mutex> lk(mtx);
    }
    cv_not_empty.notify_all();

    for (auto &c : consumers) if (c.joinable()) c.join();

    safe_print("Condition variable example: finished");
    return 0;
}
