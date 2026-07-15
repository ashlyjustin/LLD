#pragma once
#include "User.h"
#include "Location.h"

class Rider : public User {
public:
    Location currentLocation;

    Rider(std::string id, std::string name, std::string phone, Location location)
        : User(std::move(id), std::move(name), std::move(phone)),
          currentLocation(location) {}

    void updateLocation(const Location& loc) { currentLocation = loc; }
};
