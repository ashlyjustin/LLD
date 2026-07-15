# Ride Notification System — LLD Design Document

## Problem Statement

Design a notification system for a ride-hailing app (like Uber) that:
- Tracks a ride through its full lifecycle
- Sends contextual notifications to both riders and drivers
- Delivers notifications over multiple channels (Push, SMS, Email) based on user preferences
- Remains extensible — new channels, new observers, or new ride events should require minimal changes

---

## Design Patterns

| Pattern           | Where Applied                                                         | Why                                                                                      |
|-------------------|-----------------------------------------------------------------------|------------------------------------------------------------------------------------------|
| **Observer**      | `RideService` (Subject) → `IRideEventObserver` (Observer)             | Decouples ride state transitions from downstream systems like notifications, analytics   |
| **Strategy**      | `INotificationChannel` → `PushChannel`, `SMSChannel`, `EmailChannel` | Swap or add delivery channels without touching dispatch logic                            |
| **Factory**       | `NotificationFactory`                                                 | Centralises message-crafting; keeps business text out of service and channel classes     |
| **State Machine** | `Ride` class                                                          | Enforces legal transitions; throws on invalid ones before any observer is notified       |
| **Producer-Consumer** | `NotificationService` → `NotificationQueue` → `NotificationConsumer` | Decouples event detection from delivery; ride flow is never blocked by slow channels |

---

## Class Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          DOMAIN MODELS                                      │
│                                                                             │
│  ┌─────────────┐          ┌────────────────┐       ┌──────────────────┐    │
│  │    User     │◄─────────│     Ride       │──────►│     Driver       │    │
│  │─────────────│  rider   │────────────────│ driver│──────────────────│    │
│  │ id          │          │ id             │       │ vehicleNumber    │    │
│  │ name        │          │ status         │       │ vehicleModel     │    │
│  │ phone       │          │ pickupLocation │       │ currentLocation  │    │
│  │ email       │          │ dropoffLocation│       │ isAvailable      │    │
│  │ preferred   │          │ fare           │       └──────────────────┘    │
│  │  Channels[] │          │────────────────│                               │
│  └─────────────┘          │ assignDriver() │       ┌──────────────────┐    │
│        ▲                  │ startRide()    │       │    Location      │    │
│        │ extends          │ completeRide() │       │──────────────────│    │
│  ┌─────┴───────┐          │ cancelRide()   │       │ latitude         │    │
│  │   Driver    │          └────────────────┘       │ longitude        │    │
│  └─────────────┘                                   │ address          │    │
│                                                    └──────────────────┘    │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                      OBSERVER PATTERN                                       │
│                                                                             │
│  ┌──────────────────┐  subscribe()   ┌────────────────────────────────┐    │
│  │   RideService    │◄───────────────│    IRideEventObserver          │    │
│  │──────────────────│                │────────────────────────────────│    │
│  │ observers[]      │  notifies all  │ onRideRequested()  (abstract)  │    │
│  │──────────────────│───────────────►│ onDriverAssigned() (abstract)  │    │
│  │ requestRide()    │                │ onDriverArriving() (abstract)  │    │
│  │ assignDriver()   │                │ onRideStarted()    (abstract)  │    │
│  │ driverArriving() │                │ onRideCompleted()  (abstract)  │    │
│  │ startRide()      │                │ onRideCancelled()  (abstract)  │    │
│  │ completeRide()   │                └────────────────────────────────┘    │
│  │ cancelRide()     │                          ▲                           │
│  └──────────────────┘                          │ implements                │
│                                    ┌───────────┴────────────┐              │
│                                    │   NotificationService  │              │
│                                    └────────────────────────┘              │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                  STRATEGY PATTERN  (Notification Channels)                  │
│                                                                             │
│  ┌──────────────────────────┐                                               │
│  │  INotificationChannel    │  ◄── per-thread instances inside each worker  │
│  │──────────────────────────│      (no shared mutable state between threads) │
│  │ send(User, Notification) │                                               │
│  │ getType()                │                                               │
│  └──────────────────────────┘                                               │
│       ▲           ▲           ▲                                             │
│  ┌────┴───┐  ┌────┴───┐  ┌───┴────┐                                        │
│  │  Push  │  │  SMS   │  │ Email  │   ← each worker thread creates its own  │
│  │Channel │  │Channel │  │Channel │     instances; no shared state          │
│  └────────┘  └────────┘  └────────┘                                        │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                  FACTORY PATTERN                                             │
│                                                                             │
│  ┌───────────────────────────────────────────────────────┐                 │
│  │               NotificationFactory                      │                 │
│  │───────────────────────────────────────────────────────│                 │
│  │ createForRider(ride, NotificationType) → Notification  │                 │
│  │ createForDriver(ride, NotificationType) → Notification │                 │
│  └───────────────────────────────────────────────────────┘                 │
│                         │ produces                                          │
│                  ┌──────┴──────┐                                            │
│                  │ Notification │                                            │
│                  │─────────────│                                            │
│                  │ id          │                                            │
│                  │ title       │                                            │
│                  │ body        │                                            │
│                  │ type        │                                            │
│                  │ recipientId │                                            │
│                  │ timestamp   │                                            │
│                  └─────────────┘                                            │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Ride State Machine

```
                      ┌─────────────┐
             ─────────► REQUESTED   │
                      └──────┬──────┘
                             │ assignDriver()
                             ▼
                    ┌────────────────┐
                    │ DRIVER_ASSIGNED│
                    └───────┬────────┘
                            │ markDriverArriving()
                            ▼
                   ┌─────────────────┐
                   │ DRIVER_ARRIVING │
                   └────────┬────────┘
                            │ startRide()
                            ▼
                     ┌─────────────┐
                     │ IN_PROGRESS │
                     └──────┬──────┘
                            │ completeRide(fare)
                            ▼
                      ┌───────────┐
                      │ COMPLETED │
                      └───────────┘

  CANCELLED ◄── from REQUESTED, DRIVER_ASSIGNED, or DRIVER_ARRIVING only
```

**Invariants enforced by `Ride`:**
- Every transition validates the current state before mutating — throws `std::runtime_error` on violation.
- `RideService` mutates the `Ride` state **before** calling observers, so a channel failure can never roll back a transition.
- Cancellation is **not** permitted once the ride is `IN_PROGRESS` or `COMPLETED`.

---

## Full Ride Lifecycle Flow

### Scenario 1 — Happy Path (Async Queue)

```
Main Thread                                    Worker Thread (Consumer)
──────────────────────────────────────────     ──────────────────────────────────
RideService::requestRide()
  └─ Ride(REQUESTED)
  └─ NotificationService::onRideRequested()
       └─ NotificationFactory::createForRider()
       └─ NotificationQueue::push(task)  ──────►  pop() unblocks
  [returns immediately]                            dispatch → PUSH + EMAIL → Alice

RideService::assignDriver()
  └─ Ride(DRIVER_ASSIGNED)
  └─ NotificationService::onDriverAssigned()
       └─ queue.push(riderTask)  ─────────────►  dispatch → PUSH + EMAIL → Alice
       └─ queue.push(driverTask) ─────────────►  dispatch → PUSH + SMS  → Bob

RideService::driverArriving()
  └─ Ride(DRIVER_ARRIVING)
  └─ queue.push(riderTask)  ────────────────►  dispatch → PUSH + EMAIL → Alice

RideService::startRide()
  └─ Ride(IN_PROGRESS)
  └─ queue.push(riderTask)  ────────────────►  dispatch → PUSH + EMAIL → Alice

RideService::completeRide(24.50)
  └─ Ride(COMPLETED)
  └─ queue.push(riderTask)  ────────────────►  dispatch → PUSH + EMAIL → Alice (fare)
  └─ queue.push(driverTask) ────────────────►  dispatch → PUSH + SMS  → Bob (earnings)
  └─ queue.push(paymentTask)────────────────►  dispatch → PUSH + EMAIL → Alice (payment)

consumer.stop()
  └─ queue.stop() (stopped=true, notify_all)
  └─ worker drains remaining tasks
  └─ thread.join() ← main blocks here until drain complete
```

### Scenario 2 — Cancellation (before ride starts)

```
Rider                RideService            Ride           NotificationService      Queue
  │── requestRide() ──────►│── new Ride() ────►│ (REQUESTED)          │           │
  │                       │── onRideRequested──►│              push(riderTask) ───►│
  │                       │── assignDriver() ─►│ (DRIVER_ASSIGNED)    │           │
  │                       │── onDriverAssigned─►│     push(riderTask),push(driverTask)►│
  │── cancelRide() ───────►│                   │                      │           │
  │                       │── cancelRide() ───►│ (CANCELLED)          │           │
  │                       │── onRideCancelled──►│  push(riderTask),push(driverTask)►│
  │                       │                   │                      │           │
  │               [Worker drains queue — rider + driver cancel notifications sent]
```

---

## Notification Matrix

| Event               | Rider Notified | Driver Notified | Channels (defaults)                |
|---------------------|:--------------:|:---------------:|------------------------------------|
| RIDE_REQUESTED      | ✅              | ❌               | Rider: Push + Email                |
| DRIVER_ASSIGNED     | ✅              | ✅               | Rider: Push + Email / Driver: Push + SMS |
| DRIVER_ARRIVING     | ✅              | ❌               | Rider: Push + Email                |
| RIDE_STARTED        | ✅              | ❌               | Rider: Push + Email                |
| RIDE_COMPLETED      | ✅              | ✅               | Rider: Push + Email / Driver: Push + SMS |
| PAYMENT_PROCESSED   | ✅              | ❌               | Rider: Push + Email                |
| RIDE_CANCELLED      | ✅              | ✅ (if assigned) | Per preferred channels             |

---

## File Structure

```
ride_notification_system/
├── include/
│   ├── Enums.h                  # RideStatus, NotificationType, ChannelType + toString()
│   ├── Location.h               # lat/lng/address struct
│   ├── Notification.h           # Notification data class
│   ├── NotificationTask.h       # Task unit: recipient + notification (queue payload)
│   ├── NotificationQueue.h      # Thread-safe blocking FIFO queue
│   ├── NotificationConsumer.h   # Worker thread pool; owns the queue + channels
│   ├── User.h                   # Rider with preferred channels
│   ├── Driver.h                 # Extends User; adds vehicle info + location
│   ├── Ride.h                   # State machine with guarded transitions
│   ├── INotificationChannel.h   # Strategy interface
│   ├── IRideEventObserver.h     # Observer interface
│   ├── NotificationFactory.h    # Factory — crafts notification messages
│   ├── NotificationService.h    # Observer impl — enqueues tasks (pure publisher)
│   ├── RideService.h            # Subject — drives ride lifecycle
│   └── channels/
│       ├── PushChannel.h        # Push notification strategy
│       ├── SMSChannel.h         # SMS strategy
│       └── EmailChannel.h       # Email strategy
├── main.cpp                     # Demo — two scenarios
└── Makefile
```

---

## Key Design Decisions

### 1. State mutated before observers notified
`RideService` calls `ride.assignDriver()` (state change) **before** `notify(onDriverAssigned)`. A downstream failure (queue full, consumer stopped) can never roll back a ride transition.

### 2. Async delivery via Producer-Consumer queue
`NotificationService` only enqueues tasks — it never blocks on channel I/O. `NotificationConsumer` worker threads drain the queue independently. Ride events are processed in microseconds regardless of how slow a push/SMS/email provider is.

### 3. Consumer owns the queue (composition over injection)
`NotificationConsumer` holds `NotificationQueue` by value. Publishers call `consumer.enqueue()` — they never touch the queue directly. Shutdown is managed solely by the consumer, eliminating the "who owns shutdown?" ambiguity.

### 4. push() rejects after stop() — no silent loss
`NotificationQueue::push()` returns `false` once stopped. `NotificationService` logs a warning on a rejected push. Tasks already in the queue are always fully drained before worker threads exit (graceful drain).

### 5. Per-thread channel instances — zero shared state between workers
Each worker thread creates its own `PushChannel`, `SMSChannel`, and `EmailChannel`. There is no shared mutable state between threads; no channel-level mutex required.

### 6. Single worker preserves event ordering
`NotificationConsumer` defaults to **1 worker thread**. With one worker, events for the same ride are always delivered in the order they were enqueued. Using `numWorkers > 1` trades ordering guarantees for higher throughput — document this explicitly when configuring.

### 7. Cancellation restricted to pre-ride states
`Ride::cancelRide()` throws if status is `IN_PROGRESS` or `COMPLETED`. You cannot cancel a ride that is already underway — maps directly to real Uber business rules.

### 8. Multiple observers supported
Any service can subscribe without touching `RideService`:
```cpp
rideService.subscribe(&notifService);
rideService.subscribe(&analyticsService);    // future
rideService.subscribe(&surgePricingService); // future
```

---

## Async Queue — Component Detail

### NotificationQueue

```
                ┌──────────────────────────────────────────────┐
                │           NotificationQueue                   │
                │──────────────────────────────────────────────│
                │  std::queue<NotificationTask>  q              │
                │  std::mutex                    mtx            │
                │  std::condition_variable       cv             │
                │  bool                          stopped=false  │
                │──────────────────────────────────────────────│
                │  push(task) → bool                            │
                │    lock → if stopped return false             │
                │    q.push(task) → cv.notify_one()             │
                │                                               │
                │  pop() → optional<NotificationTask>           │
                │    lock → cv.wait(q not empty OR stopped)     │
                │    if q.empty() return nullopt  (exit signal) │
                │    return q.front() + q.pop()                 │
                │                                               │
                │  stop() [idempotent]                          │
                │    lock → stopped=true → cv.notify_all()      │
                └──────────────────────────────────────────────┘
```

**Shutdown sequence (graceful drain):**
```
consumer.stop()
    │
    ├─ queue.stop()
    │     ├─ stopped = true
    │     └─ cv.notify_all()  ← wakes all blocked pop() calls
    │
    └─ thread.join() × N
           │
           Worker loop:
             pop() → still has items → process them
             pop() → queue empty + stopped → returns nullopt → thread exits
```

### NotificationConsumer

```
┌─────────────────────────────────────────────────────────────┐
│                   NotificationConsumer                       │
│─────────────────────────────────────────────────────────────│
│  NotificationQueue        queue    (owned by composition)    │
│  vector<thread>           workers                           │
│  mutex                    printMtx  (stdout serialisation)  │
│─────────────────────────────────────────────────────────────│
│  enqueue(task) → bool    ← Publishers call this only        │
│  start()                 ← spawns N worker threads          │
│  stop()                  ← queue.stop() + join all threads  │
│─────────────────────────────────────────────────────────────│
│  work(workerId) [per thread]:                               │
│    channels = makeChannels()  ← per-thread, no sharing      │
│    while (task = queue.pop()) {                             │
│      for ch in task.recipient.preferredChannels:            │
│        channels[ch].send(recipient, notification)           │
│    }                                                        │
└─────────────────────────────────────────────────────────────┘
```

### NotificationTask (queue payload)

```cpp
struct NotificationTask {
    shared_ptr<User> recipient;   // who to notify + which channels
    Notification     notification; // what to say
};
```

---

## Extension Points

| What to add                       | How                                                                    |
|-----------------------------------|------------------------------------------------------------------------|
| New delivery channel (WhatsApp)   | Implement `INotificationChannel`, add to `makeChannels()` in consumer |
| New ride event (scheduled ride)   | Add method to `IRideEventObserver`, implement in `NotificationService` |
| Per-event channel preference      | Change `User::preferredChannels` to `map<NotificationType, vector<ChannelType>>` |
| Notification retry / fallback     | Add retry loop + fallback channel inside worker `dispatch` block       |
| Bounded queue / backpressure      | Add capacity check in `NotificationQueue::push()`, block or reject     |
| Multiple workers (high throughput)| `NotificationConsumer consumer(numWorkers)` — note ordering trade-off  |
| Analytics / surge pricing         | Add new `IRideEventObserver` implementation, subscribe to `RideService` |
