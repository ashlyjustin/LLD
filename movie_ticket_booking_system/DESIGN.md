# Movie Ticket Booking System — LLD

## Table of Contents
1. [Problem Statement](#1-problem-statement)
2. [Core Entities](#2-core-entities)
3. [Class Diagram](#3-class-diagram)
4. [Design Patterns](#4-design-patterns)
5. [Key Design Decisions](#5-key-design-decisions)
6. [Flows](#6-flows)
7. [Strategy Comparison](#7-strategy-comparison)
8. [Extension Points](#8-extension-points)

---

## 1. Problem Statement

Design a movie ticket booking system that allows users to:
- Browse movies and their shows
- Book a chosen number of consecutive seats of a specific type (REGULAR / PREMIUM / VIP)
- Cancel an existing booking
- Support pluggable seat-selection strategies without changing core booking logic

---

## 2. Core Entities

### Enums

| Enum | Values | Purpose |
|---|---|---|
| `SeatType` | REGULAR, PREMIUM, VIP | Classifies a physical seat |
| `SeatState` | AVAILABLE, BOOKED | Per-show occupancy state |
| `BookingStatus` | CONFIRMED, CANCELLED | Lifecycle state of a booking |

### Structs / Classes

#### `Seat`
- Immutable after `Screen` creation.
- Holds: `row`, `col`, `SeatType`, `price()`.
- No booking state — booking state lives in `Show`.

#### `Screen`
- Owns the physical seat layout as a 2D grid `seats[row][col]`.
- Rows divided into thirds on construction:
  ```
  rows 0 .. rows/3-1        → REGULAR  (Rs. 150)
  rows rows/3 .. 2*rows/3-1 → PREMIUM  (Rs. 250)
  rows 2*rows/3 .. rows-1   → VIP      (Rs. 400)
  ```
- Purely structural — no booking state, reusable across shows.

#### `Movie`
- `id`, `title`, `genre`, `durationMins`.

#### `Show`
- Links a `Movie*` and a `Screen*` with a `showTime` string.
- **Owns per-show seat states** (`vector<vector<SeatState>>`), initialised to AVAILABLE.
- Exposes query methods (`isAvailable`, `isAvailableOfType`, `countAvailable`) and mutators (`book`, `unbook`) — strategies read through these, never touching `Screen` directly.

> **Why Show owns `SeatState` and not `Seat`:**  
> The same physical screen hosts multiple shows at different times. Storing booking state on `Seat` (inside `Screen`) would make two shows conflict. `Show` isolating its own `seatStates[][]` keeps them independent.

#### `Booking`
- `bookingId`, `userId`, `Show*`, `vector<pair<int,int>> seats`, `totalAmount`, `BookingStatus`.
- Stores seat positions as `(row, col)` pairs — used to unmark on cancellation.

#### `BookingSystem` (Facade)
- Owns all entities via `unique_ptr` maps.
- Holds a `unique_ptr<SeatSelectionStrategy>` (injected at construction).
- Public API:

  | Method | Returns | Description |
  |---|---|---|
  | `addMovie(...)` | `Movie*` | Register a movie |
  | `addScreen(...)` | `Screen*` | Create a screen |
  | `addShow(...)` | `Show*` | Schedule a show |
  | `bookSeats(userId, showId, count, type)` | `optional<string>` | Book N seats; returns bookingId |
  | `cancelBooking(bookingId)` | `bool` | Cancel and free seats |
  | `getBooking(bookingId)` | `const Booking*` | Look up a booking by ID |
  | `getShowsForMovie(movieId)` | `vector<Show*>` | All shows for a movie |
  | `printShowStatus(showId)` | `void` | ASCII seat map to stdout |

---

## 3. Class Diagram

```
BookingSystem
│
├── unordered_map<string, unique_ptr<Movie>>
├── unordered_map<string, unique_ptr<Screen>>
├── unordered_map<string, unique_ptr<Show>>
├── unordered_map<string, Booking>
└── unique_ptr<SeatSelectionStrategy>
          │
          ├── ConsecutiveNearestStrategy
          ├── CenterSeatsStrategy
          └── BestAvailableStrategy
                    └── (contains CenterSeatsStrategy)

Screen ──────────────────────────────────────────────────────
│  screenId                                                  │
│  seats[row][col]  →  Seat { row, col, SeatType, price() } │
└────────────────────────────────────────────────────────────

Show ────────────────────────────────────────────────────────
│  showId, showTime                                          │
│  movie*  ──►  Movie { id, title, genre, durationMins }    │
│  screen* ──►  Screen (layout reference)                    │
│  seatStates[row][col]  →  SeatState { AVAILABLE, BOOKED } │
└────────────────────────────────────────────────────────────

Booking
│  bookingId, userId, totalAmount, BookingStatus
│  show*  ──►  Show
│  seats  →  [(row,col), ...]
```

---

## 4. Design Patterns

### Strategy Pattern — Seat Selection

```
SeatSelectionStrategy  (interface)
  └── selectSeats(Show*, count, SeatType) → vector<(row,col)>

Concrete strategies:
  ConsecutiveNearestStrategy
  CenterSeatsStrategy
  BestAvailableStrategy
```

The `BookingSystem` delegates seat selection entirely to the injected strategy. Swapping strategies requires no changes to `BookingSystem` or `Show`.

### Facade Pattern — `BookingSystem`

`BookingSystem` is the single entry point. Callers never touch `Show`, `Screen`, or `Seat` objects directly for booking operations.

### Ownership via `unique_ptr`

```
BookingSystem
  owns ──► Movie       (never deleted while shows reference them)
  owns ──► Screen      (never deleted while shows reference them)
  owns ──► Show        (never deleted while bookings reference them)
  owns ──► Booking     (value type, stored by value in map)
```

Raw pointers (`Movie*`, `Screen*`, `Show*`) are safe as long as `BookingSystem` outlives all callers — which it does since it owns everything.

---

## 5. Key Design Decisions

| Decision | Choice | Rationale |
|---|---|---|
| Booking state location | `Show` owns `seatStates[][]` | Different shows at same screen must be independent |
| `bookSeats` return type | `optional<string>` (bookingId) | Avoids stale Booking copies; caller fetches live state via `getBooking()` |
| Strategy interface | Takes `Show*` | Strategies need both physical layout (from `Screen`) and per-show availability — `Show*` provides both via query methods |
| `BestAvailableStrategy` ignores `type` | Explicit contract | Caller opts in to upgrade behaviour; it is not hidden |
| Cancellation | Idempotent-safe | Returns `false` on double-cancel; seats freed cleanly |
| Seat pricing | Per `SeatType`, global | Simple baseline; easily made show-specific by passing price override at `addShow` |

---

## 6. Flows

### 6.1 Book Seats

```
User
 │
 ▼
BookingSystem::bookSeats(userId, showId, count, type)
 │
 ├─ Look up Show* in shows map
 │    └─ return nullopt if not found
 │
 ├─ strategy->selectSeats(show, count, type)
 │    ├── ConsecutiveNearestStrategy
 │    │     Scan rows 0→N; within each row, slide a window of `count`
 │    │     Return first complete window of available seats of `type`
 │    │
 │    ├── CenterSeatsStrategy
 │    │     Collect rows of `type`; sort by |row − midRow|
 │    │     For each row: sliding window → find block whose center
 │    │     column is closest to midCol
 │    │     Return first row that has a valid block
 │    │
 │    └── BestAvailableStrategy
 │          Try VIP → PREMIUM → REGULAR via CenterSeatsStrategy
 │          Return first non-empty result (ignores requested `type`)
 │
 ├─ return nullopt if chosen is empty
 │
 ├─ For each (row, col) in chosen:
 │    show->book(row, col)        ← mutate Show's seatStates
 │    total += show->seatPriceAt(row, col)
 │
 ├─ Generate bookingId ("BKG-N")
 ├─ Store Booking in bookings map
 └─ return bookingId
```

### 6.2 Cancel Booking

```
User
 │
 ▼
BookingSystem::cancelBooking(bookingId)
 │
 ├─ Look up Booking in bookings map
 │    └─ return false if not found
 │
 ├─ Check status == CONFIRMED
 │    └─ return false if already CANCELLED
 │
 ├─ For each (row, col) in booking.seats:
 │    booking.show->unbook(row, col)   ← SeatState → AVAILABLE
 │
 ├─ booking.status = CANCELLED
 └─ return true
```

### 6.3 Add Show (Setup Flow)

```
BookingSystem::addShow(showId, movieId, screenId, time)
 │
 ├─ Resolve Movie*  from movies map  (null → return nullptr)
 ├─ Resolve Screen* from screens map (null → return nullptr)
 │
 ├─ Construct Show(showId, movie*, screen*, time)
 │    └─ Allocates seatStates[rows][cols] initialised to AVAILABLE
 │
 ├─ Store in shows map (unique_ptr)
 └─ Return Show*
```

### 6.4 Get Shows for a Movie

```
BookingSystem::getShowsForMovie(movieId)
 │
 └─ Iterate shows map, collect Show* where show->movie->id == movieId
    Return vector<Show*>
```

---

## 7. Strategy Comparison

Given a 9×10 screen (rows 0-2 REGULAR, 3-5 PREMIUM, 6-8 VIP):

| Strategy | Alice books 3 PREMIUM | Bob books 2 VIP | Behaviour |
|---|---|---|---|
| **ConsecutiveNearest** | `3:0, 3:1, 3:2` | `6:0, 6:1` | Front-left of each type zone |
| **CenterSeats** | `4:4, 4:5, 4:6` | `6:5, 6:6` | Middle row, centered columns |
| **BestAvailable** | `6:4, 6:5, 6:6` (VIP!) | `6:7, 6:8` | Upgrades to best available regardless of type |

### When to use each

- **`ConsecutiveNearestStrategy`** — maximise throughput; fill seats from front, useful when aisle-side proximity matters
- **`CenterSeatsStrategy`** — premium experience; users want the "best view" within their chosen tier
- **`BestAvailableStrategy`** — last-minute or loyalty bookings where the system offers the best seat available, possibly as an upgrade

---

## 8. Extension Points

| Feature | How to extend |
|---|---|
| Seat hold / timeout | Add `SeatState::HELD`; store hold expiry per `(show, row, col)`; run a sweeper to expire holds |
| Show-specific pricing | Pass a price matrix at `addShow`; `Show::seatPriceAt` reads it instead of global `priceFor()` |
| Multiple cinemas | Add `Cinema` entity owning `vector<Screen*>`; `BookingSystem` maps cinemaId → Cinema |
| Payment | Add `PaymentService` interface; call after `bookSeats` succeeds, rollback on failure |
| Thread safety | Mutex per `Show` for `seatStates`; `bookSeats` + `cancelBooking` hold the lock during mutation |
| Typed seat IDs | Replace `pair<int,int>` with a `SeatId { screenId, row, col }` struct for stronger invariants |
| Notifications | Observer pattern on `BookingSystem`: emit `BookingConfirmed` / `BookingCancelled` events |
