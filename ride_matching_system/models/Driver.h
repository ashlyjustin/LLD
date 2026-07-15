#pragma once
#include "User.h"
#include "Vehicle.h"
#include "Location.h"

class Driver : public User {
public:
    Vehicle  vehicle;
    Location currentLocation;
    bool     isAvailable;

    Driver(std::string id, std::string name, std::string phone,
           Vehicle vehicle, Location location)
        : User(std::move(id), std::move(name), std::move(phone)),
          vehicle(std::move(vehicle)),
          currentLocation(location),
          isAvailable(true) {}

    void updateLocation(const Location& loc)  { currentLocation = loc; }
    void setAvailability(bool available)       { isAvailable = available; }
};
