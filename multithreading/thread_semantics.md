# C++ Concurrency Deep Dive

This document is a **comprehensive, interview-oriented reference** for modern C++ concurrency primitives. It is written with **systems / infra interviews** (Cohesity, Google, large-scale backend teams) in mind and focuses on **design intent, guarantees, failure modes, and best practices**, not just API syntax.

---

## Table of Contents

1. [`std::thread`](#stdthread)

   * Lifecycle and ownership
   * `join`, `detach`, `joinable`
   * `std::terminate` and thread destruction
   * Best practices
2. [`std::terminate`](#stdterminate)

   * When it is triggered
   * Why it exists (design rationale)
   * How to avoid it
3. Mutexes and Locks

   * `std::mutex` vs `std::recursive_mutex`
   * `std::lock_guard` vs `std::unique_lock`
4. [`std::condition_variable`](#stdcondition_variable)

   * When and why to use it
   * Spurious wakeups
   * Design considerations
5. [`std::atomic`](#stdatomic)

   * When and where to use atomics
   * Memory ordering
   * Atomics vs mutexes
6. Interview Summary and Decision Guidelines

---

## `std::thread`

### What `std::thread` Represents

`std::thread` is an **RAII object that owns an OS thread**. Ownership is explicit and must be resolved before destruction.

Key properties:

* Move-only (cannot be copied)
* Owns a native thread handle
* Must be **joined or detached**

```cpp
std::thread t(worker, 42);
```


### Thread Lifecycle States

| State       | Description                         |
| ----------- | ----------------------------------- |
| Not started | Default-constructed thread          |
| Running     | Thread executing                    |
| Joinable    | Thread still owns execution         |
| Joined      | Execution finished and synchronized |
| Detached    | Execution independent of object     |

---

### `join()`
* Blocks the calling thread
* Waits until the target thread finishes
* Establishes a **happens-before relationship**

```cpp
t.join();
```
After `join()`, the thread is no longer joinable.

---

### `detach()`
* No ability to wait, synchronize, or retrieve results

```cpp
t.detach();
```

**Danger:** Detached threads can outlive referenced objects, causing undefined behavior.

---

### `joinable()`

Indicates whether the thread object still owns a thread.

    t.join();
}
```

Always guard joins in destructors.

---

`std::terminate()` is a **fail-fast mechanism** that immediately ends the program without stack unwinding.

---
### When `std::terminate` Is Triggered

1. **Destroying a joinable `std::thread`**
2. **Uncaught exception escaping a thread**
3. **Exception during stack unwinding**
4. **Throwing from a `noexcept` function**

Example:
```cpp
std::thread t(worker);
// no join or detach → std::terminate
```

---

### Why It Exists

C++ rejects silent or implicit behavior in these cases:

* Auto-joining may deadlock
* Auto-detaching may corrupt memory
* Ignoring errors leads to silent data corruption

**Design principle:** Thread and exception ownership must be explicit.

---

### Best Practices

* Always join or detach threads
* Use RAII wrappers for threads
* Catch all exceptions inside thread functions

---

## Mutexes and Locks

### `std::mutex`

* Non-recursive
* Same thread cannot lock twice
* Preferred default

```cpp
std::mutex m;
```

---

### `std::recursive_mutex`

* Allows the same thread to lock multiple times
* Higher overhead
* Masks design issues

Use only when re-entrancy is unavoidable (legacy code, callbacks).

---

### `std::lock_guard` vs `std::unique_lock`

| Feature            | lock_guard | unique_lock |
| ------------------ | ---------- | ----------- |
| Lightweight        | Yes        | No          |
| Early unlock       | No         | Yes         |
| Condition variable | No         | Yes         |
| Deferred locking   | No         | Yes         |

Use `lock_guard` by default. Use `unique_lock` when flexibility is required.

---

## `std::condition_variable`

### What Problem It Solves

Efficiently puts threads to sleep until a **shared predicate** becomes true, avoiding busy waiting.

---

### Basic Pattern

```cpp
std::mutex m;
std::condition_variable cv;
bool ready = false;

void consumer() {
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [] { return ready; });
}
```

---

### Key Concepts

* Condition variables **do not store notifications**
* The predicate is the source of truth
* Spurious wakeups are allowed

Always use `wait(lock, predicate)`.

---

### notify_one vs notify_all

| Method     | Use Case                            |
| ---------- | ----------------------------------- |
| notify_one | Single waiter                       |
| notify_all | State change affecting many waiters |

---

### Design Considerations

* Always protect the predicate with the same mutex
* Update state **before** calling notify
* Avoid multiple unrelated conditions on one CV

---

### notify_one vs notify_all — Examples

Example: `notify_one()` for a single-consumer handoff (one produced item -> one consumer):

```cpp
// producer
{
   std::lock_guard<std::mutex> lk(m);
   q.push(item);
}
not_empty.notify_one(); // wakes one waiting consumer

// consumer
std::unique_lock<std::mutex> lk(m);
not_empty.wait(lk, [&]{ return !q.empty(); });
auto item = q.front(); q.pop();
```

Example: `notify_all()` for shutdown or broadcast (wake all workers to exit):

```cpp
// main thread initiating shutdown
{
   std::lock_guard<std::mutex> lk(m);
   shutdown = true;
}
not_empty.notify_all(); // wake every waiting worker so they can exit

// worker
std::unique_lock<std::mutex> lk(m);
not_empty.wait(lk, [&]{ return shutdown || !q.empty(); });
if (shutdown && q.empty()) return; // exit
// otherwise consume
```

These examples show the intent: use `notify_one()` when exactly one waiter should proceed; use `notify_all()` when you need to broadcast a state change (commonly shutdown).

### Handling when no consumer is available (lost-notification patterns)

If no thread is currently blocked in `wait` when a producer calls `notify_one()`/`notify_all()`, the notification is not queued — it has no effect. To avoid lost notifications and ensure correctness when consumers may be busy or absent, use one of these patterns:

- Durable predicate (queue/counter): always publish state under a mutex and have consumers wait on that predicate. If consumers are busy, produced items remain in the queue and will be consumed later; no notification is required for later observation because the predicate reflects the work.

- Counting semaphore / permit-based primitive: when you need exact wake counts (N wakes for N items) use `std::counting_semaphore` (C++20) or an explicit atomic counter plus `notify_one()` calls.

- One-shot result delivery: for a single response, use `std::promise`/`std::future` to deliver the value reliably even if the consumer hasn't started waiting yet.

Minimal examples:

1) Queue + predicate (robust):

```cpp
std::queue<T> q;
std::mutex m;
std::condition_variable cv;

// producer
{
   std::lock_guard<std::mutex> lk(m);
   q.push(item); // durable state
}
cv.notify_one();

// consumer
std::unique_lock<std::mutex> lk(m);
cv.wait(lk, [&]{ return !q.empty(); }); // won't miss earlier pushes
auto it = q.front(); q.pop();
```

2) Counting semaphore (exact permits, C++20):

```cpp
#include <semaphore>
std::counting_semaphore<> sem{0};

// producer adds N permits
for (int i = 0; i < N; ++i) sem.release();

// consumer acquires one permit
sem.acquire();
```

3) One-shot delivery with promise/future (single consumer result):

```cpp
std::promise<int> p;
std::future<int> f = p.get_future();

// producer
p.set_value(42);

// consumer (even if started later)
int v = f.get();
```

Summary: never rely on notifications as the only source of truth — use durable predicates, semaphores, or futures/promises depending on the problem semantics.

## `std::atomic`

### What It Is

`std::atomic` provides **lock-free, data-race-free access** to a single variable.

```cpp
std::atomic<int> counter{0};
counter.fetch_add(1);
```

---

### When to Use Atomics

Use atomics for:

* Counters
* Flags
* Reference counts
* Simple state machines

Example:

```cpp
std::atomic<bool> shutdown{false};

while (!shutdown.load()) {
    do_work();
}
```

---

### Why Atomics Can Be Better Than Mutexes

| Aspect           | Mutex    | Atomic     |
| ---------------- | -------- | ---------- |
| Blocking         | Yes      | No         |
| Context switches | Possible | None       |
| Deadlocks        | Possible | Impossible |
| Overhead         | Higher   | Very low   |

Atomics compile to CPU instructions instead of kernel calls.

---

### Memory Ordering (Overview)

* **Sequential consistency (default):** safest, slowest
* **Relaxed:** counters, metrics
* **Acquire/Release:** publishing data safely

```cpp
ready.store(true, std::memory_order_release);
if (ready.load(std::memory_order_acquire)) {
    use(data);
}
```

---

### When NOT to Use Atomics

* Multiple related variables
* Complex invariants
* Logic requiring blocking or waiting

In such cases, prefer mutexes and condition variables.

---

## Decision Guidelines (Interview-Ready)

| Problem                 | Preferred Tool                |
| ----------------------- | ----------------------------- |
| Mutual exclusion        | `std::mutex`                  |
| Scoped locking          | `std::lock_guard`             |
| Waiting for state       | `std::condition_variable`     |
| Simple counters / flags | `std::atomic`                 |
| Recursive locking       | `std::recursive_mutex` (rare) |

---

## Final Takeaways

* `std::terminate` enforces explicit ownership and fail-fast safety
* Condition variables wait on **state**, not signals
* Atomics are not a replacement for mutexes, but a powerful complement
* Correct concurrency design is about **choosing the right primitive**, not just using threads

This README can be used as:

* Interview preparation material
* Internal team reference
* Foundation for implementing thread pools, queues, and schedulers
