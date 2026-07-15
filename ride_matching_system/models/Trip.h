#pragma once
#include <string>
#include <ctime>
#include "Location.h"
#include "Vehicle.h"

enum class TripStatus {
    REQUESTED,
    DRIVER_ASSIGNED,
    IN_PROGRESS,
    COMPLETED,
    CANCELLED
};

inline std::string tripStatusToString(TripStatus s) {
    switch (s) {
        case TripStatus::REQUESTED:       return "REQUESTED";
        case TripStatus::DRIVER_ASSIGNED: return "DRIVER_ASSIGNED";
        case TripStatus::IN_PROGRESS:     return "IN_PROGRESS";
        case TripStatus::COMPLETED:       return "COMPLETED";
        case TripStatus::CANCELLED:       return "CANCELLED";
    }
    return "UNKNOWN";
}

// Trip stores only IDs for driver/rider — actual objects live in their services.
// Pickup/drop and vehicleType are snapshotted at creation time.
class Trip {
public:
    std::string id;
    std::string riderId;
    std::string driverId;       // set once driver is assigned

    Location    pickupLocation;
    Location    dropLocation;
    VehicleType vehicleType;    // snapshot from request

    TripStatus  status;
    double      fare        = 0.0;
    double      distanceKm  = 0.0;
    double      durationMin = 0.0;

    std::time_t requestedAt  = 0;
    std::time_t startedAt    = 0;
    std::time_t completedAt  = 0;

    bool driverRated = false;
    bool riderRated  = false;

    Trip(std::string id, std::string riderId,
         Location pickup, Location drop, VehicleType vehicleType)
        : id(std::move(id)), riderId(std::move(riderId)),
          pickupLocation(pickup), dropLocation(drop),
          vehicleType(vehicleType),
          status(TripStatus::REQUESTED),
          requestedAt(std::time(nullptr)) {}
};
