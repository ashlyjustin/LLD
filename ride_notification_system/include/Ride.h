#pragma once
#include <string>
#include <memory>
#include <stdexcept>
#include "Enums.h"
#include "User.h"
#include "Driver.h"
#include "Location.h"

/*
 * Ride state machine:
 *
 *   REQUESTED ──► DRIVER_ASSIGNED ──► DRIVER_ARRIVING ──► IN_PROGRESS ──► COMPLETED
 *       │                │                  │
 *       └────────────────┴──────────────────┴──► CANCELLED
 *
 * Cancellation is only allowed before the ride is IN_PROGRESS.
 */
class Ride {
    std::string id;
    std::shared_ptr<User>   rider;
    std::shared_ptr<Driver> driver;
    Location pickupLocation;
    Location dropoffLocation;
    RideStatus status;
    double fare;

    void assertStatus(RideStatus expected, const std::string& action) const {
        if (status != expected)
            throw std::runtime_error(action + ": invalid state transition from " + toString(status));
    }

public:
    Ride(std::string id, std::shared_ptr<User> rider,
         Location pickup, Location dropoff)
        : id(std::move(id))
        , rider(std::move(rider))
        , pickupLocation(std::move(pickup))
        , dropoffLocation(std::move(dropoff))
        , status(RideStatus::REQUESTED)
        , fare(0.0) {}

    // ── Accessors ──────────────────────────────────────────────────────────
    const std::string&      getId()       const { return id; }
    std::shared_ptr<User>   getRider()    const { return rider; }
    std::shared_ptr<Driver> getDriver()   const { return driver; }
    const Location&         getPickup()   const { return pickupLocation; }
    const Location&         getDropoff()  const { return dropoffLocation; }
    RideStatus              getStatus()   const { return status; }
    double                  getFare()     const { return fare; }

    // ── State transitions ──────────────────────────────────────────────────
    void assignDriver(std::shared_ptr<Driver> d) {
        assertStatus(RideStatus::REQUESTED, "assignDriver");
        driver = std::move(d);
        status = RideStatus::DRIVER_ASSIGNED;
    }

    void markDriverArriving() {
        assertStatus(RideStatus::DRIVER_ASSIGNED, "markDriverArriving");
        status = RideStatus::DRIVER_ARRIVING;
    }

    void startRide() {
        assertStatus(RideStatus::DRIVER_ARRIVING, "startRide");
        status = RideStatus::IN_PROGRESS;
    }

    void completeRide(double rideFare) {
        assertStatus(RideStatus::IN_PROGRESS, "completeRide");
        fare   = rideFare;
        status = RideStatus::COMPLETED;
    }

    // Cancellation is not allowed once the ride is IN_PROGRESS or COMPLETED.
    void cancelRide() {
        if (status == RideStatus::IN_PROGRESS || status == RideStatus::COMPLETED)
            throw std::runtime_error("cancelRide: cannot cancel a ride that is " + toString(status));
        status = RideStatus::CANCELLED;
    }
};
