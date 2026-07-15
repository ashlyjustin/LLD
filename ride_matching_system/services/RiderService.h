#pragma once
#include "../models/Rider.h"
#include <unordered_map>
#include <memory>
#include <stdexcept>

class RiderService {
public:
    void addRider(std::shared_ptr<Rider> rider) {
        riders_[rider->id] = std::move(rider);
    }

    std::shared_ptr<Rider> getRider(const std::string& id) const {
        auto it = riders_.find(id);
        if (it == riders_.end())
            throw std::runtime_error("Rider not found: " + id);
        return it->second;
    }

    void updateLocation(const std::string& riderId, const Location& loc) {
        getRider(riderId)->updateLocation(loc);
    }

private:
    std::unordered_map<std::string, std::shared_ptr<Rider>> riders_;
};
