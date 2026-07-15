#pragma once
#include "../models/Driver.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <stdexcept>

class DriverService {
public:
    void addDriver(std::shared_ptr<Driver> driver) {
        drivers_[driver->id] = std::move(driver);
    }

    std::shared_ptr<Driver> getDriver(const std::string& id) const {
        auto it = drivers_.find(id);
        if (it == drivers_.end())
            throw std::runtime_error("Driver not found: " + id);
        return it->second;
    }

    void updateLocation(const std::string& driverId, const Location& loc) {
        getDriver(driverId)->updateLocation(loc);
    }

    void setAvailability(const std::string& driverId, bool available) {
        getDriver(driverId)->setAvailability(available);
    }

    std::vector<std::shared_ptr<Driver>> getAllDrivers() const {
        std::vector<std::shared_ptr<Driver>> result;
        result.reserve(drivers_.size());
        for (const auto& [id, d] : drivers_) result.push_back(d);
        return result;
    }

    std::vector<std::shared_ptr<Driver>> getAvailableDrivers(VehicleType type) const {
        std::vector<std::shared_ptr<Driver>> result;
        for (const auto& [id, d] : drivers_)
            if (d->isAvailable && d->vehicle.type == type) result.push_back(d);
        return result;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<Driver>> drivers_;
};
