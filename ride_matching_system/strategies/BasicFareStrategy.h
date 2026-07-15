#pragma once
#include "FareStrategy.h"
#include <unordered_map>

// Per-vehicle-type rate table.
// Fare = baseFare + (perKmRate * distance) + (perMinRate * duration)
class BasicFareStrategy : public FareStrategy {
public:
    BasicFareStrategy() {
        rates_[VehicleType::BIKE]  = {10.0,  5.0, 0.50};
        rates_[VehicleType::AUTO]  = {15.0,  8.0, 0.75};
        rates_[VehicleType::SEDAN] = {25.0, 12.0, 1.00};
        rates_[VehicleType::SUV]   = {40.0, 18.0, 1.50};
    }

    double calculateFare(double distanceKm, double durationMin,
                         VehicleType vehicleType) const override {
        auto it = rates_.find(vehicleType);
        if (it == rates_.end()) return 0.0;
        const auto& r = it->second;
        return r.baseFare + (r.perKmRate * distanceKm) + (r.perMinRate * durationMin);
    }

private:
    struct RateConfig { double baseFare, perKmRate, perMinRate; };
    std::unordered_map<VehicleType, RateConfig> rates_;
};
