#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "include/User.h"
#include "include/Driver.h"
#include "include/Ride.h"
#include "include/Location.h"
#include "include/NotificationConsumer.h"
#include "include/NotificationService.h"
#include "include/RideService.h"

static void printSeparator(const std::string& label) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "  " << label << "\n";
    std::cout << std::string(60, '=') << "\n";
}

int main() {
    std::cout << "=== Uber-like Ride Notification System (Async Queue) ===\n";

    // ── Infrastructure setup ───────────────────────────────────────────────
    //
    // NotificationConsumer owns the queue.
    // numWorkers=1 preserves per-ride event ordering.
    // Use numWorkers>1 only when ordering between events is not critical.
    NotificationConsumer consumer(/*numWorkers=*/1);
    consumer.start();

    NotificationService notifService(consumer);
    RideService         rideService;
    rideService.subscribe(&notifService);

    // ── Users ──────────────────────────────────────────────────────────────
    auto rider = std::make_shared<User>(
        "U001", "Alice", "+1-555-0101", "alice@email.com",
        std::vector<ChannelType>{ChannelType::PUSH, ChannelType::EMAIL});

    auto driver = std::make_shared<Driver>(
        "D001", "Bob", "+1-555-0202", "bob@email.com",
        "KA01AB1234", "Toyota Prius");
    driver->setLocation(Location{12.9716, 77.5946, "MG Road, Bangalore"});

    Location pickup {12.9352, 77.6245, "Koramangala, Bangalore"};
    Location dropoff{12.9698, 77.7499, "Whitefield, Bangalore"};

    // ══════════════════════════════════════════════════════════════════════
    //  SCENARIO 1 — Full ride lifecycle
    // ══════════════════════════════════════════════════════════════════════
    printSeparator("SCENARIO 1: Full Ride Lifecycle");

    std::cout << "\n>> Step 1: Rider requests a ride\n";
    auto ride = rideService.requestRide("RIDE_001", rider, pickup, dropoff);

    std::cout << "\n>> Step 2: Driver assigned\n";
    rideService.assignDriver(*ride, driver);

    std::cout << "\n>> Step 3: Driver arriving at pickup\n";
    rideService.driverArriving(*ride);

    std::cout << "\n>> Step 4: Ride started\n";
    rideService.startRide(*ride);

    std::cout << "\n>> Step 5: Ride completed (fare $24.50)\n";
    rideService.completeRide(*ride, 24.50);

    // ══════════════════════════════════════════════════════════════════════
    //  SCENARIO 2 — Cancellation after driver assigned
    // ══════════════════════════════════════════════════════════════════════
    printSeparator("SCENARIO 2: Cancellation After Driver Assigned");

    auto rider2 = std::make_shared<User>(
        "U002", "Charlie", "+1-555-0303", "charlie@email.com",
        std::vector<ChannelType>{ChannelType::PUSH, ChannelType::SMS});

    auto driver2 = std::make_shared<Driver>(
        "D002", "Diana", "+1-555-0404", "diana@email.com",
        "KA02CD5678", "Honda City");

    auto ride2 = rideService.requestRide("RIDE_002", rider2, pickup, dropoff);

    std::cout << "\n>> Assigning driver to RIDE_002\n";
    rideService.assignDriver(*ride2, driver2);

    std::cout << "\n>> Rider cancels RIDE_002\n";
    rideService.cancelRide(*ride2);

    // ── Graceful drain & shutdown ──────────────────────────────────────────
    //
    // All events have been enqueued. stop() lets the consumer drain every
    // pending task before joining worker threads — no notifications are lost.
    std::cout << "\n[main] All ride events enqueued. Waiting for consumer to drain...\n";
    consumer.stop();

    std::cout << "\n=== System Demo Complete ===\n";
    return 0;
}
