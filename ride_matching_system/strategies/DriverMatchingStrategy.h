#pragma once
#include "../models/Driver.h"
#include "../models/Location.h"
#include <vector>
#include <memory>

// Strategy interface for selecting a driver for a given pickup location.
class DriverMatchingStrategy {
public:
    virtual ~DriverMatchingStrategy() = default;

    // Returns the best available driver, or nullptr if none found.
    virtual std::shared_ptr<Driver> findBestDriver(
        const Location& pickup,
        const std::vector<std::shared_ptr<Driver>>& candidates,
        VehicleType vehicleType) const = 0;
};
