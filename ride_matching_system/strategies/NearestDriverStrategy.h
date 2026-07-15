#pragma once
#include "DriverMatchingStrategy.h"
#include <limits>

// Picks the closest available driver of the requested vehicle type.
class NearestDriverStrategy : public DriverMatchingStrategy {
public:
    std::shared_ptr<Driver> findBestDriver(
        const Location& pickup,
        const std::vector<std::shared_ptr<Driver>>& candidates,
        VehicleType vehicleType) const override {

        std::shared_ptr<Driver> best;
        double minDist = std::numeric_limits<double>::max();

        for (const auto& driver : candidates) {
            if (!driver->isAvailable || driver->vehicle.type != vehicleType) continue;
            double dist = driver->currentLocation.distanceTo(pickup);
            if (dist < minDist) {
                minDist = dist;
                best    = driver;
            }
        }
        return best;
    }
};
