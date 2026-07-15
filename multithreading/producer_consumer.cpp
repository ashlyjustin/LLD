#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <memory>
#include <vector>
#include <chrono>

std::mutex cout_mtx;
void safe_print(const std::string &s) {
    std::lock_guard<std::mutex> lk(cout_mtx);
    std::cout << s << std::endl;
}

// Class-based bounded-buffer producer-consumer example

template<typename T>
class BoundedBuffer {
public:
    explicit BoundedBuffer(size_t capacity) : capacity_(capacity), closed_(false) {}

    // push returns false if buffer is closed
    bool push(T item) {
        std::unique_lock<std::mutex> lk(m_);
        not_full_.wait(lk, [&]{ return q_.size() < capacity_ || closed_; });
        if (closed_) return false;
        q_.push(std::move(item));
        not_empty_.notify_one();
        return true;
    }

    // pop returns false if buffer is closed and empty
    bool pop(T &out) {
        std::unique_lock<std::mutex> lk(m_);
        not_empty_.wait(lk, [&]{ return !q_.empty() || closed_; });
        if (q_.empty()) return false; // closed and empty
        out = std::move(q_.front()); q_.pop();
        not_full_.notify_one();
        return true;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lk(m_);
            closed_ = true;
        }
        not_empty_.notify_all();
        not_full_.notify_all();
    }

private:
    std::queue<T> q_;
    size_t capacity_;
    std::mutex m_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    bool closed_;
};

class Producer {
public:
    Producer(int id, std::atomic<int> &next, int total, BoundedBuffer<int> &buf)
        : id_(id), next_(next), total_(total), buf_(buf) {}

    void start() { th_ = std::thread(&Producer::run, this); }
    void join() { if (th_.joinable()) th_.join(); }

private:
    void run() {
        while (true) {
            int i = next_.fetch_add(1, std::memory_order_relaxed);
            if (i >= total_) break;
            if (!buf_.push(i)) break; // buffer closed
            safe_print("producer " + std::to_string(id_) + " pushed " + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
    }

    int id_, total_;
    BoundedBuffer<int> &buf_;
    std::thread th_;
    std::atomic<int> &next_;
};

class Consumer {
public:
    Consumer(int id, BoundedBuffer<int> &buf) : id_(id), buf_(buf) {}
    void start() { th_ = std::thread(&Consumer::run, this); }
    void join() { if (th_.joinable()) th_.join(); }

private:
    void run() {
        while (true) {
            int v;
            if (!buf_.pop(v)) break; // buffer closed and empty
            safe_print("consumer " + std::to_string(id_) + " popped " + std::to_string(v));
            // simulate work
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    int id_;
    BoundedBuffer<int> &buf_;
    std::thread th_;
};

int main() {
    safe_print("Producer-Consumer (class-based) starting");

    const size_t buffer_capacity = 5;
    BoundedBuffer<int> buffer(buffer_capacity);

    const int total_items = 30;
    const int producer_count = 2;
    const int consumer_count = 3;

    std::vector<std::unique_ptr<Producer>> producers;
    std::vector<std::unique_ptr<Consumer>> consumers;

    std::atomic<int> next{0};

    for (int i = 0; i < producer_count; ++i) {
        producers.emplace_back(std::make_unique<Producer>(i, next, total_items, buffer));
    }
    for (int i = 0; i < consumer_count; ++i) {
        consumers.emplace_back(std::make_unique<Consumer>(i, buffer));
    }

    for (auto &c : consumers) c->start();
    for (auto &p : producers) p->start();

    for (auto &p : producers) p->join();

    // signal consumers to finish after producers are done
    buffer.close();

    for (auto &c : consumers) c->join();

    safe_print("Producer-Consumer finished");
    return 0;
}
