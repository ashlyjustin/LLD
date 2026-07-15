#pragma once
#include "FareStrategy.h"
#include <memory>
#include <stdexcept>

// Wraps another FareStrategy and applies a surge multiplier to the total fare.
class SurgeFareStrategy : public FareStrategy {
public:
    SurgeFareStrategy(std::shared_ptr<FareStrategy> base, double multiplier)
        : base_(std::move(base)), surgeMultiplier_(multiplier) {
        if (multiplier < 1.0)
            throw std::invalid_argument("Surge multiplier must be >= 1.0");
    }

    double calculateFare(double distanceKm, double durationMin,
                         VehicleType vehicleType) const override {
        return base_->calculateFare(distanceKm, durationMin, vehicleType) * surgeMultiplier_;
    }

    double getSurgeMultiplier() const { return surgeMultiplier_; }

private:
    std::shared_ptr<FareStrategy> base_;
    double surgeMultiplier_;
};
