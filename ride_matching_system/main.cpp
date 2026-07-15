#include <iostream>
#include <iomanip>
#include <memory>
#include "models/Location.h"
#include "models/Driver.h"
#include "models/Rider.h"
#include "models/Vehicle.h"
#include "models/Trip.h"
#include "strategies/BasicFareStrategy.h"
#include "strategies/SurgeFareStrategy.h"
#include "strategies/NearestDriverStrategy.h"
#include "services/DriverService.h"
#include "services/RiderService.h"
#include "services/TripService.h"
#include "services/FareCalculationService.h"
#include "services/RideMatchingService.h"
#include "observers/NotificationObserver.h"

// ─── Helpers ──────────────────────────────────────────────────────────────────

static void header(const std::string& title) {
    std::cout << "\n" << std::string(65, '=') << "\n"
              << "  " << title << "\n"
              << std::string(65, '=') << "\n";
}

static void printTrip(const Trip& t) {
    std::cout << "\n  ┌─ Trip Summary ───────────────────────────────────────\n"
              << "  │  ID       : " << t.id << "\n"
              << "  │  Rider    : " << t.riderId << "\n"
              << "  │  Driver   : " << (t.driverId.empty() ? "N/A" : t.driverId) << "\n"
              << "  │  Status   : " << tripStatusToString(t.status) << "\n"
              << "  │  Vehicle  : " << vehicleTypeToString(t.vehicleType) << "\n"
              << "  │  Pickup   : " << t.pickupLocation.toString() << "\n"
              << "  │  Drop     : " << t.dropLocation.toString() << "\n";
    if (t.status == TripStatus::COMPLETED)
        std::cout << "  │  Distance : " << std::fixed << std::setprecision(2) << t.distanceKm << " km\n"
                  << "  │  Duration : " << t.durationMin << " min\n"
                  << "  │  Fare     : $" << t.fare << "\n";
    std::cout << "  └──────────────────────────────────────────────────────\n";
}

// ─── Main ─────────────────────────────────────────────────────────────────────

int main() {
    // ── Bootstrap services ────────────────────────────────────────────────────
    DriverService          driverService;
    RiderService           riderService;
    auto                   fareStrategy = std::make_shared<BasicFareStrategy>();
    FareCalculationService fareService(fareStrategy);
    TripService            tripService(driverService, riderService);
    auto                   matchStrategy = std::make_shared<NearestDriverStrategy>();
    RideMatchingService    rideService(driverService, riderService,
                                       tripService, fareService, matchStrategy);

    NotificationObserver notifier;
    tripService.addObserver(&notifier);

    // ── Register drivers ──────────────────────────────────────────────────────
    header("Registering Drivers & Riders");

    driverService.addDriver(std::make_shared<Driver>(
        "D001", "Alice Johnson", "+1-555-0101",
        Vehicle("V001", "Honda City",     "KA-01-AB-1234", VehicleType::SEDAN),
        Location(12.9716, 77.5946)));   // MG Road

    driverService.addDriver(std::make_shared<Driver>(
        "D002", "Bob Smith",    "+1-555-0102",
        Vehicle("V002", "Toyota Innova",  "KA-02-CD-5678", VehicleType::SUV),
        Location(12.9352, 77.6245)));   // Koramangala

    driverService.addDriver(std::make_shared<Driver>(
        "D003", "Charlie Davis", "+1-555-0103",
        Vehicle("V003", "Honda Activa",   "KA-03-EF-9012", VehicleType::BIKE),
        Location(12.9279, 77.6271)));   // HSR Layout

    driverService.addDriver(std::make_shared<Driver>(
        "D004", "Diana Prince", "+1-555-0104",
        Vehicle("V004", "Suzuki Swift",   "KA-04-GH-3456", VehicleType::SEDAN),
        Location(12.9850, 77.5533)));   // Malleshwaram

    riderService.addRider(std::make_shared<Rider>(
        "R001", "Eve Williams", "+1-555-0201", Location(12.9700, 77.5950)));
    riderService.addRider(std::make_shared<Rider>(
        "R002", "Frank Miller", "+1-555-0202", Location(12.9360, 77.6240)));
    riderService.addRider(std::make_shared<Rider>(
        "R003", "Grace Lee",    "+1-555-0203", Location(12.9400, 77.6200)));

    std::cout << "  4 drivers registered: Alice (Sedan), Bob (SUV), Charlie (Bike), Diana (Sedan)\n"
              << "  3 riders  registered: Eve, Frank, Grace\n";

    // ── Scenario 1: Normal Sedan ride (request → assign → start → end → rate) ─
    header("Scenario 1: Eve requests a Sedan ride");
    {
        auto result = rideService.requestRide(
            "R001",
            Location(12.9716, 77.5946),   // MG Road
            Location(12.9279, 77.6271),   // HSR Layout
            VehicleType::SEDAN);

        if (result) {
            auto& trip = *result;
            rideService.startRide(trip->id, trip->driverId);
            rideService.endRide(trip->id, trip->driverId, 25.0);
            rideService.rateDriver(trip->id, 4.8);
            rideService.rateRider(trip->id, 4.5);
            printTrip(*trip);
        }
    }

    // ── Scenario 2: SUV ride ───────────────────────────────────────────────────
    header("Scenario 2: Frank requests an SUV ride");
    {
        auto result = rideService.requestRide(
            "R002",
            Location(12.9300, 77.6250),
            Location(13.0100, 77.5500),   // Hebbal
            VehicleType::SUV);

        if (result) {
            auto& trip = *result;
            rideService.startRide(trip->id, trip->driverId);
            rideService.endRide(trip->id, trip->driverId, 35.0);
            rideService.rateDriver(trip->id, 4.2);
            rideService.rateRider(trip->id, 5.0);
            printTrip(*trip);
        }
    }

    // ── Scenario 3: Surge pricing (1.5×) ─────────────────────────────────────
    header("Scenario 3: Surge pricing active (1.5×) — Grace requests Sedan");
    {
        fareService.setStrategy(std::make_shared<SurgeFareStrategy>(fareStrategy, 1.5));

        auto result = rideService.requestRide(
            "R003",
            Location(12.9400, 77.6200),
            Location(12.9716, 77.5946),   // Back to MG Road
            VehicleType::SEDAN);

        if (result) {
            auto& trip = *result;
            rideService.startRide(trip->id, trip->driverId);
            rideService.endRide(trip->id, trip->driverId, 20.0);
            printTrip(*trip);
        }

        fareService.setStrategy(fareStrategy); // restore normal pricing
    }

    // ── Scenario 4: Rider cancels after driver is assigned ───────────────────
    header("Scenario 4: Eve cancels after driver is assigned");
    {
        auto result = rideService.requestRide(
            "R001",
            Location(12.9716, 77.5946),
            Location(12.9500, 77.6000),
            VehicleType::SEDAN);

        if (result) {
            auto& trip = *result;
            std::cout << "  [ACTION] Eve cancels the ride...\n";
            rideService.cancelRide(trip->id);
            printTrip(*trip);

            // Verify driver is available again
            auto driver = driverService.getDriver(trip->driverId);
            std::cout << "  [CHECK ] Driver " << driver->name
                      << " availability restored: "
                      << (driver->isAvailable ? "YES" : "NO") << "\n";
        }
    }

    // ── Scenario 5: No driver of requested type available ────────────────────
    header("Scenario 5: No Bike drivers available");
    {
        driverService.setAvailability("D003", false); // make Charlie busy
        auto result = rideService.requestRide(
            "R002",
            Location(12.9716, 77.5946),
            Location(12.9500, 77.6000),
            VehicleType::BIKE);
        if (!result) std::cout << "  [CHECK ] Confirmed: no bike driver found.\n";
        driverService.setAvailability("D003", true);  // restore
    }

    // ── Scenario 6: Duplicate rating guard ───────────────────────────────────
    header("Scenario 6: Attempting to rate driver twice (guard test)");
    {
        auto result = rideService.requestRide(
            "R003",
            Location(12.9400, 77.6200),
            Location(12.9800, 77.5700),
            VehicleType::SEDAN);

        if (result) {
            auto& trip = *result;
            rideService.startRide(trip->id, trip->driverId);
            rideService.endRide(trip->id, trip->driverId, 15.0);
            rideService.rateDriver(trip->id, 4.0);
            try {
                rideService.rateDriver(trip->id, 3.0); // should throw
                std::cout << "  [FAIL  ] Duplicate rating was allowed!\n";
            } catch (const std::exception& e) {
                std::cout << "  [PASS  ] Duplicate rating blocked: " << e.what() << "\n";
            }
        }
    }

    // ── Trip history ──────────────────────────────────────────────────────────
    header("Rider Trip History — Eve (R001)");
    for (const auto& t : tripService.getRiderTripHistory("R001"))
        printTrip(*t);

    header("Driver Trip History — Alice (D001)");
    for (const auto& t : tripService.getDriverTripHistory("D001"))
        printTrip(*t);

    // ── Driver ratings ────────────────────────────────────────────────────────
    header("Final Driver Ratings");
    for (const auto& d : driverService.getAllDrivers()) {
        std::cout << "  " << std::left << std::setw(16) << d->name
                  << " (" << d->id << "): "
                  << std::fixed << std::setprecision(1) << d->getRating()
                  << " ★  (" << d->getRatingCount() << " rating"
                  << (d->getRatingCount() != 1 ? "s" : "") << ")\n";
    }

    std::cout << "\n" << std::string(65, '=') << "\n"
              << "  Ride Matching System — Demo Complete\n"
              << std::string(65, '=') << "\n\n";
    return 0;
}
