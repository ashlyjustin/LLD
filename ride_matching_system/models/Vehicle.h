#pragma once
#include <string>

enum class VehicleType { BIKE, AUTO, SEDAN, SUV };

inline std::string vehicleTypeToString(VehicleType type) {
    switch (type) {
        case VehicleType::BIKE:  return "Bike";
        case VehicleType::AUTO:  return "Auto";
        case VehicleType::SEDAN: return "Sedan";
        case VehicleType::SUV:   return "SUV";
    }
    return "Unknown";
}

struct Vehicle {
    std::string id;
    std::string model;
    std::string licensePlate;
    VehicleType type;

    Vehicle(std::string id, std::string model, std::string plate, VehicleType type)
        : id(std::move(id)), model(std::move(model)),
          licensePlate(std::move(plate)), type(type) {}
};
