/*
std::thread lifecycle (join vs detach)

std::mutex, std::recursive_mutex

std::lock_guard vs std::unique_lock

std::condition_variable

std::atomic<T> (memory ordering basics)
*/

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>

#include <condition_variable>

std::mutex mtx;
int shared_counter = 0;

std::recursive_mutex rec_mtx;
std::mutex cout_mtx; // protect std::cout

std::condition_variable cv;
std::mutex cv_mtx;
bool reached_target = false;

// (atomic usage removed from this practice file)

void safe_print(const std::string &s) {
	std::lock_guard<std::mutex> lock(cout_mtx);
	std::cout << s << std::endl;
}

// Demonstrate std::lock_guard (simple, RAII)
void worker_lock_guard(int id, int increments) {
	for (int i = 0; i < increments; ++i) {
		{
			std::lock_guard<std::mutex> lock(mtx);
			++shared_counter;
		}
		if ((i + 1) % (increments / 4 + 1) == 0) {
			safe_print("[lock_guard] worker " + std::to_string(id) + " progressed");
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

// Demonstrate std::unique_lock (can lock/unlock manually)
void worker_unique_lock(int id, int increments) {
	for (int i = 0; i < increments; ++i) {
		std::unique_lock<std::mutex> lock(mtx);
		++shared_counter;
		// temporarily release the lock while we do non-critical work
		lock.unlock();

		if ((i + 1) % (increments / 4 + 1) == 0) {
			safe_print("[unique_lock] worker " + std::to_string(id) + " progressed");
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

// Demonstrate recursive_mutex: a function that locks recursively
void recursive_function(int depth) {
	if (depth <= 0) return;
	std::lock_guard<std::recursive_mutex> lock(rec_mtx);
	safe_print("recursive depth " + std::to_string(depth));
	if (depth > 1) recursive_function(depth - 1);
}

// Worker that notifies a condition variable when shared_counter crosses target
void notifier_worker(int target) {
	while (true) {
		{
			std::lock_guard<std::mutex> lock(mtx);
			if (shared_counter >= target) break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	{
		std::lock_guard<std::mutex> lk(cv_mtx);
		reached_target = true;
	}
	cv.notify_one();
	safe_print("notifier: target reached and notified");
}

// (detached/atomic worker removed — atomic deep-dive lives in a separate example file)

int main() {
	safe_print("Starting multithreading practice program");

	const int per_worker = 300;

	// Condition variable: wait until shared_counter >= target
	int target = per_worker * 2 / 2; // midway
	std::thread notifier(notifier_worker, target);
    
	// Thread lifecycle: create threads and join them
	std::thread t1(worker_lock_guard, 1, per_worker);
	std::thread t2(worker_unique_lock, 2, per_worker);


	// Recursive mutex demo in its own thread
	std::thread trec([](){ recursive_function(4); });

	// Wait for condition variable notification
	{
		std::unique_lock<std::mutex> lk(cv_mtx);
		cv.wait(lk, []{ return reached_target; });
	}
	safe_print("main: received notification that shared_counter reached target");

	// Join joinable threads
	if (t1.joinable()) t1.join();
	if (t2.joinable()) t2.join();
	if (notifier.joinable()) notifier.join();
	if (trec.joinable()) trec.join();

	safe_print("Final shared_counter (mutex-protected): " + std::to_string(shared_counter));

	safe_print("Program finished cleanly");
	return 0;
}

