#pragma once
#include <string>
#include <vector>
#include "User.h"
#include "Location.h"

class Driver : public User {
    std::string vehicleNumber;
    std::string vehicleModel;
    double rating;
    bool available;
    Location currentLocation;

public:
    Driver(std::string id, std::string name, std::string phone, std::string email,
           std::string vehicleNumber, std::string vehicleModel,
           std::vector<ChannelType> channels = {ChannelType::PUSH, ChannelType::SMS})
        : User(std::move(id), std::move(name), std::move(phone), std::move(email), std::move(channels))
        , vehicleNumber(std::move(vehicleNumber))
        , vehicleModel(std::move(vehicleModel))
        , rating(5.0)
        , available(true) {}

    const std::string& getVehicleNumber() const { return vehicleNumber; }
    const std::string& getVehicleModel()  const { return vehicleModel; }
    double getRating()   const { return rating; }
    bool   isAvailable() const { return available; }

    void setAvailable(bool val)           { available = val; }
    void setLocation(const Location& loc) { currentLocation = loc; }
    const Location& getCurrentLocation()  const { return currentLocation; }
};
