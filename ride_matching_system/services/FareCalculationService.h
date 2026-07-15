#pragma once
#include "../strategies/FareStrategy.h"
#include <memory>

class FareCalculationService {
public:
    explicit FareCalculationService(std::shared_ptr<FareStrategy> strategy)
        : strategy_(std::move(strategy)) {}

    void setStrategy(std::shared_ptr<FareStrategy> strategy) {
        strategy_ = std::move(strategy);
    }

    double calculateFare(double distanceKm, double durationMin,
                         VehicleType vehicleType) const {
        return strategy_->calculateFare(distanceKm, durationMin, vehicleType);
    }

private:
    std::shared_ptr<FareStrategy> strategy_;
};
