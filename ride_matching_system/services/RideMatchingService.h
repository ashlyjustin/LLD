#pragma once
#include "DriverService.h"
#include "RiderService.h"
#include "TripService.h"
#include "FareCalculationService.h"
#include "../strategies/DriverMatchingStrategy.h"
#include <memory>
#include <optional>
#include <iostream>
#include <stdexcept>

// Top-level orchestrator that ties together driver matching, trip lifecycle,
// and fare calculation. All trip state changes are delegated to TripService.
class RideMatchingService {
public:
    RideMatchingService(DriverService&                        driverService,
                        RiderService&                         riderService,
                        TripService&                          tripService,
                        FareCalculationService&               fareService,
                        std::shared_ptr<DriverMatchingStrategy> matchingStrategy)
        : driverService_(driverService),
          riderService_(riderService),
          tripService_(tripService),
          fareService_(fareService),
          matchingStrategy_(std::move(matchingStrategy)) {}

    void setMatchingStrategy(std::shared_ptr<DriverMatchingStrategy> strategy) {
        matchingStrategy_ = std::move(strategy);
    }

    // Request a ride: creates a Trip, finds nearest driver, assigns them.
    // Returns the Trip on success, or std::nullopt if no driver is available.
    std::optional<std::shared_ptr<Trip>> requestRide(
        const std::string& riderId,
        const Location&    pickup,
        const Location&    destination,
        VehicleType        vehicleType = VehicleType::SEDAN) {

        riderService_.getRider(riderId); // validate rider exists

        auto trip   = tripService_.createTrip(riderId, pickup, destination, vehicleType);
        auto driver = matchingStrategy_->findBestDriver(
            pickup, driverService_.getAllDrivers(), vehicleType);

        if (!driver) {
            tripService_.cancelTrip(trip->id);
            std::cout << "[SYSTEM] No available " << vehicleTypeToString(vehicleType)
                      << " drivers. Trip " << trip->id << " cancelled.\n";
            return std::nullopt;
        }

        tripService_.assignDriver(trip->id, driver->id);

        double eta = driver->currentLocation.distanceTo(pickup) / 0.5; // ~30 km/h → 0.5 km/min
        std::cout << "[SYSTEM] Driver " << driver->name
                  << " assigned to rider " << riderId
                  << " (ETA: ~" << static_cast<int>(eta) << " min)\n";
        return trip;
    }

    // DRIVER_ASSIGNED → IN_PROGRESS (called when driver starts the ride)
    void startRide(const std::string& tripId, const std::string& driverId) {
        auto trip = tripService_.getTrip(tripId);
        requireAssignedDriver(trip, driverId);
        tripService_.startTrip(tripId);
    }

    // IN_PROGRESS → COMPLETED; fare is calculated here and stored in Trip
    std::shared_ptr<Trip> endRide(const std::string& tripId,
                                  const std::string& driverId,
                                  double             durationMin) {
        auto trip = tripService_.getTrip(tripId);
        requireAssignedDriver(trip, driverId);

        double distance = trip->pickupLocation.distanceTo(trip->dropLocation);
        double fare     = fareService_.calculateFare(distance, durationMin, trip->vehicleType);
        tripService_.completeTrip(tripId, fare, distance, durationMin);
        return trip;
    }

    // Cancel a trip (allowed before IN_PROGRESS)
    void cancelRide(const std::string& tripId) {
        tripService_.cancelTrip(tripId);
    }

    void rateDriver(const std::string& tripId, double rating) {
        tripService_.rateDriver(tripId, rating);
    }

    void rateRider(const std::string& tripId, double rating) {
        tripService_.rateRider(tripId, rating);
    }

private:
    DriverService&                        driverService_;
    RiderService&                         riderService_;
    TripService&                          tripService_;
    FareCalculationService&               fareService_;
    std::shared_ptr<DriverMatchingStrategy> matchingStrategy_;

    void requireAssignedDriver(const std::shared_ptr<Trip>& trip,
                               const std::string& driverId) const {
        if (trip->driverId != driverId)
            throw std::runtime_error(
                "Driver " + driverId + " is not assigned to trip " + trip->id);
    }
};
