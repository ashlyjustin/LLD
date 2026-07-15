#pragma once
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include "Ride.h"
#include "IRideEventObserver.h"

/*
 * RideService — Subject in the Observer pattern.
 *
 * Responsibilities:
 *   1. Drive the Ride state machine (request → assign → arriving → start → complete/cancel).
 *   2. Notify all registered observers after each successful state transition.
 *
 * Observers are stored as raw pointers. Callers must ensure observer lifetime
 * exceeds the lifetime of this service (standard RAII guarantee in main()).
 */
class RideService {
    std::vector<IRideEventObserver*> observers;

    void notify(void (IRideEventObserver::*fn)(const Ride&), const Ride& ride) {
        for (auto* obs : observers) {
            try {
                (obs->*fn)(ride);
            } catch (const std::exception& e) {
                // Observer failures must not roll back ride transitions.
                std::cerr << "[WARN] Observer notification failed: " << e.what() << "\n";
            }
        }
    }

public:
    // ── Observer registration ───────────────────────────────────────────────

    void subscribe(IRideEventObserver* observer) {
        observers.push_back(observer);
    }

    void unsubscribe(IRideEventObserver* observer) {
        observers.erase(std::remove(observers.begin(), observers.end(), observer),
                        observers.end());
    }

    // ── Ride lifecycle methods ──────────────────────────────────────────────

    std::shared_ptr<Ride> requestRide(const std::string& rideId,
                                      std::shared_ptr<User> rider,
                                      const Location& pickup,
                                      const Location& dropoff) {
        auto ride = std::make_shared<Ride>(rideId, std::move(rider), pickup, dropoff);
        notify(&IRideEventObserver::onRideRequested, *ride);
        return ride;
    }

    void assignDriver(Ride& ride, std::shared_ptr<Driver> driver) {
        ride.assignDriver(std::move(driver));          // mutate state first
        notify(&IRideEventObserver::onDriverAssigned, ride);
    }

    void driverArriving(Ride& ride) {
        ride.markDriverArriving();
        notify(&IRideEventObserver::onDriverArriving, ride);
    }

    void startRide(Ride& ride) {
        ride.startRide();
        notify(&IRideEventObserver::onRideStarted, ride);
    }

    void completeRide(Ride& ride, double fare) {
        ride.completeRide(fare);
        notify(&IRideEventObserver::onRideCompleted, ride);
    }

    void cancelRide(Ride& ride) {
        ride.cancelRide();
        notify(&IRideEventObserver::onRideCancelled, ride);
    }
};
