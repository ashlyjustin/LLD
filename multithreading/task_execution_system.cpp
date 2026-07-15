/*
Implement a multi threaded task execution system in C++17 that supports task submission, execution, and result retrieval using futures and promises. 
The system should manage a pool of worker threads to execute submitted tasks concurrently.
*/

#include "thread_pool.h"
#include <iostream>
#include <future>

int main() {
    ThreadPool pool(4); // 4 worker threads

    // Submit tasks
    auto f1 = pool.submit([]() { return 100; });
    auto f2 = pool.submit([](int x) { return x * 2; }, 21);
    auto f3 = pool.submit([]() { std::cout << "Task 3 running\n"; });

    // Retrieve results
    std::cout << "Result 1: " << f1.get() << "\n";
    std::cout << "Result 2: " << f2.get() << "\n";
    f3.get(); // void task

    // Submit more tasks with promises
    std::promise<int> prom;
    auto fut = prom.get_future();
    pool.submit([&prom]() { prom.set_value(555); });
    std::cout << "Promise result: " << fut.get() << "\n";

    return 0;
}

