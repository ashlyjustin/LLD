#pragma once
#include <string>

struct Location {
    double latitude;
    double longitude;
    std::string address;

    Location() : latitude(0.0), longitude(0.0) {}
    Location(double lat, double lng, std::string addr = "")
        : latitude(lat), longitude(lng), address(std::move(addr)) {}
};
