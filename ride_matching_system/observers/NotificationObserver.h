#pragma once
#include "TripObserver.h"
#include <iostream>
#include <iomanip>

class NotificationObserver : public TripObserver {
public:
    void onTripStatusChanged(const Trip& trip) override {
        std::cout << "[NOTIFICATION] Trip " << trip.id
                  << " → " << tripStatusToString(trip.status);

        switch (trip.status) {
            case TripStatus::REQUESTED:
                std::cout << " | Rider: " << trip.riderId
                          << " | From: " << trip.pickupLocation.toString()
                          << " → " << trip.dropLocation.toString();
                break;
            case TripStatus::DRIVER_ASSIGNED:
                std::cout << " | Driver: " << trip.driverId;
                break;
            case TripStatus::IN_PROGRESS:
                std::cout << " | Ride started";
                break;
            case TripStatus::COMPLETED:
                std::cout << " | Distance: " << std::fixed << std::setprecision(2)
                          << trip.distanceKm << " km"
                          << " | Fare: $" << trip.fare;
                break;
            case TripStatus::CANCELLED:
                std::cout << " | Cancelled";
                break;
        }
        std::cout << "\n";
    }
};
