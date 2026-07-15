#pragma once
#include "../models/Vehicle.h"

// Strategy interface for fare calculation.
// Fare = (baseFare + perKm*distance + perMin*duration) * surge
// The surge multiplier is baked into concrete strategies.
class FareStrategy {
public:
    virtual ~FareStrategy() = default;
    virtual double calculateFare(double distanceKm, double durationMin,
                                 VehicleType vehicleType) const = 0;
};
