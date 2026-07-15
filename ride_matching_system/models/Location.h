#pragma once
#include <string>
#include <cmath>

struct Location {
    double latitude;
    double longitude;

    Location(double lat = 0.0, double lon = 0.0) : latitude(lat), longitude(lon) {}

    // Haversine formula — returns distance in km
    double distanceTo(const Location& other) const {
        constexpr double R = 6371.0;
        double lat1 = latitude  * M_PI / 180.0;
        double lat2 = other.latitude  * M_PI / 180.0;
        double dLat = (other.latitude  - latitude)  * M_PI / 180.0;
        double dLon = (other.longitude - longitude) * M_PI / 180.0;

        double a = std::sin(dLat / 2) * std::sin(dLat / 2)
                 + std::cos(lat1) * std::cos(lat2)
                 * std::sin(dLon / 2) * std::sin(dLon / 2);
        double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
        return R * c;
    }

    std::string toString() const {
        return "(" + std::to_string(latitude) + ", " + std::to_string(longitude) + ")";
    }
};
