/*
Rate limiter implementation using token bucket algorithm.
Ensure multiple threads can safely acquire tokens at a limited rate.
Uses C++17 standard features with interface-based design for algorithm injection.
*/

#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <stdexcept>

static std::mutex cout_mtx;
static void safePrint(const std::string &s) {
    std::lock_guard<std::mutex> lk(cout_mtx);
    std::cout << s << std::endl;
}

// Abstract interface for rate limiter algorithms
class IRateLimiter {
public:
    virtual ~IRateLimiter() = default;
    virtual bool try_acquire(int tokens) = 0;           // non-blocking
    virtual bool acquire(int tokens) = 0;               // blocking
};

// Token bucket rate limiter implementation
class TokenBucketRateLimiter : public IRateLimiter {
public:
    TokenBucketRateLimiter(int max_tokens, int refill_rate)
        : max_tokens_(max_tokens), tokens_(max_tokens), refill_rate_(refill_rate), stop_(false) {
        refill_thread_ = std::thread(&TokenBucketRateLimiter::refill, this);
        safePrint("TokenBucketRateLimiter: started (max=" + std::to_string(max_tokens_) + ", refill=" + std::to_string(refill_rate_) + ")");
    }

    ~TokenBucketRateLimiter() override {
        safePrint("TokenBucketRateLimiter: destructor");
        stop_ = true;
        refill_cv_.notify_all();
        if (refill_thread_.joinable()) {
            refill_thread_.join();
        }
    }

    // Non-blocking attempt: return immediately if insufficient tokens
    bool try_acquire(int tokens) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (tokens_ >= tokens) {
            tokens_ -= tokens;
            safePrint("try_acquire: granted " + std::to_string(tokens) + " remaining=" + std::to_string(tokens_));
            return true;
        }
        safePrint("try_acquire: denied requested=" + std::to_string(tokens) + " available=" + std::to_string(tokens_));
        return false;
    }

    // Blocking acquire: waits until tokens available or stop signal
    bool acquire(int tokens) override {
        std::unique_lock<std::mutex> lock(mutex_);
        // Wait until: stop is signaled OR enough tokens available
        refill_cv_.wait(lock, [&]{ return stop_.load() || tokens_ >= tokens; });
        if (stop_.load()) {
            safePrint("acquire: interrupted by stop");
            return false;
        }
        tokens_ -= tokens;
        safePrint("acquire: granted " + std::to_string(tokens) + " remaining=" + std::to_string(tokens_));
        return true;
    }

    void stop() {
        safePrint("TokenBucketRateLimiter: stop() called");
        stop_ = true;
        refill_cv_.notify_all();
        if (refill_thread_.joinable()) {
            refill_thread_.join();
        }
    }

private:
    void refill() {
        while (!stop_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(700));
            std::unique_lock<std::mutex> lock(mutex_);
            int before = tokens_;
            tokens_ = std::min(max_tokens_, tokens_ + refill_rate_);
            int after = tokens_;
            lock.unlock();
            safePrint("refill: before=" + std::to_string(before) + " after=" + std::to_string(after));
            refill_cv_.notify_all();
        }
    }

    const int max_tokens_;
    int tokens_;
    const int refill_rate_;
    std::thread refill_thread_;
    std::mutex mutex_;
    std::condition_variable refill_cv_;
    std::atomic<bool> stop_;
};

// Service wrapper - receives injected dependency
class RateLimiterService {
public:
    explicit RateLimiterService(std::unique_ptr<IRateLimiter> limiter) : limiter_(std::move(limiter)) {}
    
    bool try_acquire(int tokens) {
        return limiter_->try_acquire(tokens);
    }

private:
    std::unique_ptr<IRateLimiter> limiter_;
};

int main() {
    // 1. Instantiate concrete algorithm once
    auto token_bucket = std::make_unique<TokenBucketRateLimiter>(10, 5); // max 10 tokens, refill 5 tokens/sec
    
    // 2. Inject dependency into service
    RateLimiterService service(std::move(token_bucket));

    // 3. Pass injected service to workers
    auto worker = [&](int id) {
        for (int i = 0; i < 5; ++i) {
            if (service.try_acquire(3)) { // try to acquire 3 tokens
                // Simulate work
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            } else {
                safePrint("worker " + std::to_string(id) + " failed to acquire tokens, will retry");
                std::this_thread::sleep_for(std::chrono::milliseconds(700));
            }
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(worker, i);
    }

    for (auto &th : threads) {
        if (th.joinable()) th.join();
    }

    return 0;
}