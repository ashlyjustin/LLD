#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>

std::mutex cout_mtx;
void safe_print(const std::string &s) {
    std::lock_guard<std::mutex> lk(cout_mtx);
    std::cout << s << std::endl;
}

// Demonstrates simple atomic counters and acquire/release synchronization
int main() {
    safe_print("Atomic example: starting");

    // Example 1: atomic counter with relaxed ordering
    std::atomic<int> relaxed_counter{0};
    const int threads_count = 4;
    const int increments = 5000;

    std::vector<std::thread> t;
    for (int i = 0; i < threads_count; ++i) {
        t.emplace_back([&]{
            for (int j = 0; j < increments; ++j) {
                relaxed_counter.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto &th : t) if (th.joinable()) th.join();
    safe_print("relaxed_counter expected " + std::to_string(threads_count * increments) + ", got " + std::to_string(relaxed_counter.load()));

    // Example 2: release/acquire ordering (producer-consumer handoff)
    int data = 0; // non-atomic data protected by ordering
    std::atomic<bool> ready{false};

    std::thread producer([&]{
        data = 42; // write data first
        ready.store(true, std::memory_order_release);
        safe_print("producer: stored data and set ready (release)");
    });

    std::thread consumer([&]{
        while (!ready.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        // after acquire load returns true, we can safely read `data`
        safe_print(std::string("consumer: observed ready (acquire), data=") + std::to_string(data));
    });

    producer.join();
    consumer.join();

    safe_print("Atomic example: finished");
    return 0;
}
