# Ride Matching System — Design Document

---

## Table of Contents

1. [Problem Statement](#1-problem-statement)
2. [High-Level Architecture](#2-high-level-architecture)
3. [Project Structure](#3-project-structure)
4. [Class Design](#4-class-design)
   - [Models](#41-models)
   - [Strategies](#42-strategies)
   - [Services](#43-services)
   - [Observers](#44-observers)
5. [Design Patterns](#5-design-patterns)
6. [Flows](#6-flows)
   - [Request a Ride](#61-request-a-ride)
   - [Start a Ride](#62-start-a-ride)
   - [End a Ride](#63-end-a-ride)
   - [Cancel a Ride](#64-cancel-a-ride)
   - [Surge Pricing](#65-surge-pricing)
   - [Rating Flow](#66-rating-flow)
7. [Trip State Machine](#7-trip-state-machine)
8. [Fare Calculation](#8-fare-calculation)
9. [Observer Flow](#9-observer-flow)
10. [Key Design Decisions](#10-key-design-decisions)

---

## 1. Problem Statement

Design a ride-hailing system (similar to Uber) that supports:

- Registering drivers and riders
- Requesting a ride with a vehicle preference
- Matching the nearest available driver to a rider
- Managing trip lifecycle (assign → start → complete / cancel)
- Calculating fare (with optional surge pricing)
- Rating drivers and riders after trip completion
- Notifying observers on every trip status change
- Viewing trip history per rider and per driver

---

## 2. High-Level Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                        main.cpp  (client)                        │
└───────────────────────────┬──────────────────────────────────────┘
                            │ uses
                            ▼
┌──────────────────────────────────────────────────────────────────┐
│                    RideMatchingService                            │
│   (top-level orchestrator — entry point for all rider actions)   │
└──────┬──────────────┬──────────────┬────────────────┬────────────┘
       │              │              │                │
       ▼              ▼              ▼                ▼
 DriverService   RiderService   TripService   FareCalculationService
       │                            │                │
       │                            │ notifies       ▼
       │                            ▼          FareStrategy
       │                      TripObserver      (BasicFare /
       │                      (observers)        SurgeFare)
       ▼
 DriverMatchingStrategy
 (NearestDriver / ...)
```

---

## 3. Project Structure

```
ride_matching_system/
│
├── models/
│   ├── Location.h              — lat/lon + Haversine distance
│   ├── User.h                  — base class: id, name, phone, rating
│   ├── Driver.h                — User + Vehicle + location + availability
│   ├── Rider.h                 — User + location
│   ├── Vehicle.h               — VehicleType enum + vehicle info
│   └── Trip.h                  — TripStatus enum + all trip data
│
├── strategies/
│   ├── FareStrategy.h          — interface: calculateFare()
│   ├── BasicFareStrategy.h     — per-vehicle rate table
│   ├── SurgeFareStrategy.h     — wraps any FareStrategy with a multiplier
│   ├── DriverMatchingStrategy.h — interface: findBestDriver()
│   └── NearestDriverStrategy.h — picks closest available driver
│
├── services/
│   ├── DriverService.h         — driver CRUD + availability + location
│   ├── RiderService.h          — rider CRUD + location
│   ├── FareCalculationService.h — delegates to pluggable FareStrategy
│   ├── TripService.h           — owns full trip lifecycle + state machine + observers
│   └── RideMatchingService.h   — orchestrates matching, fare, trip transitions
│
├── observers/
│   ├── TripObserver.h          — interface: onTripStatusChanged(Trip&)
│   └── NotificationObserver.h  — concrete: prints formatted alerts to console
│
├── main.cpp                    — demo with 6 scenarios
└── Makefile
```

---

## 4. Class Design

### 4.1 Models

#### `Location`
```
Location
├── latitude  : double
├── longitude : double
└── distanceTo(other) → double   // Haversine, returns km
```

#### `User` (base class)
```
User
├── id, name, phone : string
├── addRating(double)
├── getRating()      → double    // average; defaults to 5.0 if unrated
└── getRatingCount() → int
```
> **Note:** `virtual ~User()` ensures safe deletion through base pointer.

#### `Driver` extends `User`
```
Driver
├── vehicle         : Vehicle
├── currentLocation : Location
├── isAvailable     : bool
├── updateLocation(Location)
└── setAvailability(bool)
```

#### `Rider` extends `User`
```
Rider
├── currentLocation : Location
└── updateLocation(Location)
```

#### `Vehicle`
```
VehicleType : enum { BIKE, AUTO, SEDAN, SUV }

Vehicle
├── id, model, licensePlate : string
└── type : VehicleType
```

#### `Trip`
```
TripStatus : enum { REQUESTED, DRIVER_ASSIGNED, IN_PROGRESS, COMPLETED, CANCELLED }

Trip
├── id, riderId, driverId   : string   ← IDs only (no raw pointers to objects)
├── pickupLocation, dropLocation : Location   ← snapshotted at creation
├── vehicleType             : VehicleType     ← snapshotted at creation
├── status                  : TripStatus
├── fare, distanceKm, durationMin : double
├── requestedAt, startedAt, completedAt : time_t
└── driverRated, riderRated : bool           ← one-rating-per-trip guards
```

> **Design note:** `Trip` stores IDs, not `shared_ptr` to Driver/Rider.  
> This avoids shared ownership confusion and dangling-reference bugs.  
> Actual objects are resolved through services when needed.

---

### 4.2 Strategies

#### `FareStrategy` (interface)
```cpp
virtual double calculateFare(double distanceKm,
                             double durationMin,
                             VehicleType) const = 0;
```

#### `BasicFareStrategy`
Per-vehicle rate table:

| Vehicle | Base (₹) | Per km (₹) | Per min (₹) |
|---------|----------|------------|-------------|
| Bike    | 10       | 5          | 0.50        |
| Auto    | 15       | 8          | 0.75        |
| Sedan   | 25       | 12         | 1.00        |
| SUV     | 40       | 18         | 1.50        |

Formula: `fare = baseFare + (perKm × distance) + (perMin × duration)`

#### `SurgeFareStrategy`
Wraps any `FareStrategy` and multiplies the result:
```
fare = wrapped.calculateFare(...) × surgeMultiplier
```
Multiplier must be ≥ 1.0.

#### `DriverMatchingStrategy` (interface)
```cpp
virtual shared_ptr<Driver> findBestDriver(
    const Location& pickup,
    const vector<shared_ptr<Driver>>& candidates,
    VehicleType) const = 0;
```

#### `NearestDriverStrategy`
Iterates all candidates, filters by `isAvailable && vehicleType == requested`,
returns the one with minimum Haversine distance to pickup.

---

### 4.3 Services

#### `DriverService`
| Method | Description |
|--------|-------------|
| `addDriver(Driver)` | Register a new driver |
| `getDriver(id)` | Lookup by ID (throws if not found) |
| `updateLocation(id, Location)` | Update driver's GPS |
| `setAvailability(id, bool)` | Mark available / busy |
| `getAllDrivers()` | Returns all drivers |
| `getAvailableDrivers(VehicleType)` | Filter by type + availability |

#### `RiderService`
| Method | Description |
|--------|-------------|
| `addRider(Rider)` | Register a new rider |
| `getRider(id)` | Lookup by ID (throws if not found) |
| `updateLocation(id, Location)` | Update rider's GPS |

#### `FareCalculationService`
- Holds a `shared_ptr<FareStrategy>`
- `setStrategy()` allows hot-swapping (e.g. surge during peak hours)
- `calculateFare(distance, duration, vehicleType)` → delegates to strategy

#### `TripService`
The **single source of truth** for all trip state.

| Method | Transition |
|--------|-----------|
| `createTrip(...)` | → `REQUESTED` |
| `assignDriver(tripId, driverId)` | `REQUESTED` → `DRIVER_ASSIGNED` |
| `startTrip(tripId)` | `DRIVER_ASSIGNED` → `IN_PROGRESS` |
| `completeTrip(tripId, fare, dist, dur)` | `IN_PROGRESS` → `COMPLETED` |
| `cancelTrip(tripId)` | `REQUESTED/DRIVER_ASSIGNED` → `CANCELLED` |
| `rateDriver(tripId, rating)` | Trip must be COMPLETED; one-time |
| `rateRider(tripId, rating)` | Trip must be COMPLETED; one-time |
| `getRiderTripHistory(riderId)` | Returns all trips for a rider |
| `getDriverTripHistory(driverId)` | Returns all trips for a driver |

Every state change:
1. Validates the transition against the state machine
2. Mutates the `Trip` object
3. Calls `notify()` → fires all registered `TripObserver`s

#### `RideMatchingService`
Orchestrator — exposes the high-level rider/driver API:

| Method | Description |
|--------|-------------|
| `requestRide(riderId, pickup, dest, type)` | Find driver + create + assign trip |
| `startRide(tripId, driverId)` | Validate driver identity + start trip |
| `endRide(tripId, driverId, duration)` | Calculate fare + complete trip |
| `cancelRide(tripId)` | Cancel at any pre-in-progress state |
| `rateDriver(tripId, rating)` | Post-trip driver rating |
| `rateRider(tripId, rating)` | Post-trip rider rating |

---

### 4.4 Observers

#### `TripObserver` (interface)
```cpp
virtual void onTripStatusChanged(const Trip& trip) = 0;
```

#### `NotificationObserver`
Prints a formatted `[NOTIFICATION]` line per status, with status-specific context:

| Status | Extra info printed |
|--------|--------------------|
| `REQUESTED` | Rider ID, pickup, drop |
| `DRIVER_ASSIGNED` | Driver ID |
| `IN_PROGRESS` | "Ride started" |
| `COMPLETED` | Distance, fare |
| `CANCELLED` | "Cancelled" |

---

## 5. Design Patterns

### Strategy Pattern

Used in **two** places:

```
FareStrategy ◄──── BasicFareStrategy
                └── SurgeFareStrategy (wraps another FareStrategy)

DriverMatchingStrategy ◄──── NearestDriverStrategy
```

**Why:** Fare calculation and driver selection algorithms can change independently
at runtime without modifying any service. New algorithms (e.g. `RatingWeightedStrategy`,
`MinEtaStrategy`) are added by implementing the interface.

---

### Observer Pattern

```
TripService (Subject)
    │  addObserver(TripObserver*)
    │  notify(trip) on every state change
    │
    ├──► NotificationObserver::onTripStatusChanged()
    ├──► LoggingObserver::onTripStatusChanged()       ← easy to add
    ├──► BillingObserver::onTripStatusChanged()       ← easy to add
    └──► AnalyticsObserver::onTripStatusChanged()     ← easy to add
```

**Why:** `TripService` has zero knowledge of what happens after a status change.
Notification, logging, billing etc. are plugged in externally — Open/Closed Principle.

---

### Single Responsibility

| Class | Responsibility |
|-------|---------------|
| `TripService` | Trip state machine + history + observer dispatch |
| `RideMatchingService` | Ride request orchestration |
| `FareCalculationService` | Fare delegation |
| `DriverService` | Driver data + availability |
| `NearestDriverStrategy` | Driver selection algorithm |
| `NotificationObserver` | Console notification formatting |

---

## 6. Flows

### 6.1 Request a Ride

```
Rider calls: rideService.requestRide("R001", pickup, dest, SEDAN)

RideMatchingService
    │
    ├─1─► RiderService::getRider("R001")          — validate rider exists
    │
    ├─2─► TripService::createTrip(...)
    │         └─ Trip created with status = REQUESTED
    │         └─ notify(trip) → [NOTIFICATION] REQUESTED
    │
    ├─3─► DriverService::getAllDrivers()           — fetch candidates
    │
    ├─4─► NearestDriverStrategy::findBestDriver(pickup, candidates, SEDAN)
    │         └─ filter: isAvailable == true && vehicleType == SEDAN
    │         └─ return driver with min distanceTo(pickup)
    │
    ├─5a─► [No driver found]
    │         └─ TripService::cancelTrip(tripId)
    │         └─ return std::nullopt
    │
    └─5b─► [Driver found]
              └─ TripService::assignDriver(tripId, driverId)
                     └─ status: REQUESTED → DRIVER_ASSIGNED
                     └─ DriverService::setAvailability(driverId, false)
                     └─ notify(trip) → [NOTIFICATION] DRIVER_ASSIGNED
              └─ return Trip
```

---

### 6.2 Start a Ride

```
Driver calls: rideService.startRide(tripId, driverId)

RideMatchingService
    │
    ├─1─► TripService::getTrip(tripId)
    ├─2─► Validate: trip.driverId == driverId        — prevents wrong driver
    └─3─► TripService::startTrip(tripId)
              └─ status: DRIVER_ASSIGNED → IN_PROGRESS
              └─ startedAt = now()
              └─ notify(trip) → [NOTIFICATION] IN_PROGRESS
```

---

### 6.3 End a Ride

```
Driver calls: rideService.endRide(tripId, driverId, durationMin)

RideMatchingService
    │
    ├─1─► TripService::getTrip(tripId)
    ├─2─► Validate: trip.driverId == driverId
    ├─3─► Compute distance = pickup.distanceTo(drop)   — Haversine
    ├─4─► FareCalculationService::calculateFare(distance, duration, vehicleType)
    │         └─ delegates to BasicFareStrategy (or SurgeFareStrategy if active)
    └─5─► TripService::completeTrip(tripId, fare, distance, duration)
              └─ status: IN_PROGRESS → COMPLETED
              └─ fare, distanceKm, durationMin stored on Trip
              └─ completedAt = now()
              └─ DriverService::setAvailability(driverId, true)   — driver freed
              └─ notify(trip) → [NOTIFICATION] COMPLETED | Fare: $X
```

---

### 6.4 Cancel a Ride

```
Rider calls: rideService.cancelRide(tripId)

RideMatchingService
    └─► TripService::cancelTrip(tripId)
            │
            ├─ If status == REQUESTED:
            │       └─ status → CANCELLED
            │       └─ notify(trip) → [NOTIFICATION] CANCELLED
            │
            └─ If status == DRIVER_ASSIGNED:
                    └─ status → CANCELLED
                    └─ DriverService::setAvailability(driverId, true)  ← driver freed
                    └─ notify(trip) → [NOTIFICATION] CANCELLED

    ✗  Cannot cancel if status == IN_PROGRESS or later (throws exception)
```

---

### 6.5 Surge Pricing

```
// Activate surge (e.g. peak hours, rain)
fareService.setStrategy(
    make_shared<SurgeFareStrategy>(basicFareStrategy, 1.5)
);

// Any endRide() call now uses:
fare = basicFare(distance, duration, vehicleType) × 1.5

// Deactivate
fareService.setStrategy(basicFareStrategy);
```

No trip or service code changes — only the strategy is swapped.

---

### 6.6 Rating Flow

```
Rider rates driver: rideService.rateDriver(tripId, 4.8)

RideMatchingService
    └─► TripService::rateDriver(tripId, 4.8)
            │
            ├─ Validate: trip.status == COMPLETED       (throws otherwise)
            ├─ Validate: trip.driverRated == false       (one-time guard)
            ├─ Validate: 1.0 <= rating <= 5.0
            ├─ DriverService::getDriver(trip.driverId)->addRating(4.8)
            │       └─ ratingSum += 4.8 ; ratingCount++
            └─ trip.driverRated = true

Driver's average = ratingSum / ratingCount
```

Same flow for `rateRider()` in reverse.

---

## 7. Trip State Machine

```
                    ┌──────────────┐
                    │  REQUESTED   │
                    └──────┬───────┘
                           │ assignDriver()
               ┌───────────▼──────────┐
               │   DRIVER_ASSIGNED    │
               └───────────┬──────────┘
                           │ startTrip()
               ┌───────────▼──────────┐
               │     IN_PROGRESS      │
               └───────────┬──────────┘
                           │ completeTrip()
               ┌───────────▼──────────┐
               │      COMPLETED       │  ← terminal
               └──────────────────────┘

  cancelTrip() allowed from: REQUESTED, DRIVER_ASSIGNED
  ┌──────────────┐
  │  CANCELLED   │  ← terminal
  └──────────────┘
```

Invalid transitions throw `std::runtime_error`. The valid transition table:

| From | Allowed next states |
|------|-------------------|
| `REQUESTED` | `DRIVER_ASSIGNED`, `CANCELLED` |
| `DRIVER_ASSIGNED` | `IN_PROGRESS`, `CANCELLED` |
| `IN_PROGRESS` | `COMPLETED` |
| `COMPLETED` | *(none — terminal)* |
| `CANCELLED` | *(none — terminal)* |

---

## 8. Fare Calculation

### Formula

```
fare = baseFare + (perKmRate × distanceKm) + (perMinRate × durationMin)
```

With surge:
```
fare = above_formula × surgeMultiplier
```

Surge is applied to the **entire fare** (including base), not just the variable components.

### Rate Table

| Vehicle | Base | Per km | Per min | Example (10 km, 20 min) |
|---------|------|--------|---------|------------------------|
| Bike    | ₹10  | ₹5     | ₹0.50   | 10 + 50 + 10 = **₹70**  |
| Auto    | ₹15  | ₹8     | ₹0.75   | 15 + 80 + 15 = **₹110** |
| Sedan   | ₹25  | ₹12    | ₹1.00   | 25 + 120 + 20 = **₹165**|
| SUV     | ₹40  | ₹18    | ₹1.50   | 40 + 180 + 30 = **₹250**|

---

## 9. Observer Flow

```
TripService                    Observers
    │                              │
    │  createTrip()                │
    ├──── notify(REQUESTED) ──────►│ NotificationObserver.onTripStatusChanged()
    │                              │     prints: Trip X → REQUESTED | Rider: ...
    │  assignDriver()              │
    ├──── notify(DRIVER_ASSIGNED) ►│     prints: Trip X → DRIVER_ASSIGNED | Driver: D001
    │                              │
    │  startTrip()                 │
    ├──── notify(IN_PROGRESS) ────►│     prints: Trip X → IN_PROGRESS | Ride started
    │                              │
    │  completeTrip()              │
    ├──── notify(COMPLETED) ──────►│     prints: Trip X → COMPLETED | Distance: X km | Fare: $Y
    │                              │
    │  cancelTrip()                │
    └──── notify(CANCELLED) ──────►│     prints: Trip X → CANCELLED
```

### Adding a new observer

```cpp
class BillingObserver : public TripObserver {
public:
    void onTripStatusChanged(const Trip& trip) override {
        if (trip.status == TripStatus::COMPLETED)
            chargeRider(trip.riderId, trip.fare);
    }
};

// Register
BillingObserver billing;
tripService.addObserver(&billing);
```

`TripService` requires **no modification**.

---

## 10. Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| `Trip` stores IDs, not `shared_ptr<Driver/Rider>` | Avoids shared ownership / cycles; services are the owners |
| `TripService` is the sole trip state machine owner | One place enforces transitions; prevents split-brain between services |
| `virtual ~User() = default` on base class | Safe polymorphic deletion — prevents UB when deleting via `User*` |
| `inline` on free functions (`tripStatusToString`) | Avoids ODR (One Definition Rule) violations across multiple translation units |
| Rating stored as `sum + count` | Correct incremental average; also detects unrated users vs truly rated at 5.0 |
| `std::optional` return from `requestRide` | Forces caller to handle the no-driver case explicitly — no silent null |
| Surge as a decorator over `FareStrategy` | Surge can wrap any future strategy without duplicating logic |
| State machine as `static const map` in `TripService` | Single definition of allowed transitions; easy to audit and extend |

---

*Build: `make` — Run: `make run`*
