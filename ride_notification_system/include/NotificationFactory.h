#pragma once
#include <memory>
#include <sstream>
#include <iomanip>
#include "Notification.h"
#include "Ride.h"

/*
 * Factory Pattern: builds Notification objects for a given ride event.
 * Keeps message-crafting logic in one place, separate from dispatch logic.
 */
class NotificationFactory {
    inline static int counter = 0;

    static std::string nextId() {
        return "NOTIF_" + std::to_string(++counter);
    }

    static std::string formatFare(double fare) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << fare;
        return "$" + oss.str();
    }

public:
    // Notification addressed to the rider
    static std::unique_ptr<Notification> createForRider(
            const Ride& ride, NotificationType type) {

        std::string title, body;

        switch (type) {
            case NotificationType::RIDE_REQUESTED:
                title = "Ride Requested";
                body  = "Looking for a nearby driver for your pickup at "
                        + ride.getPickup().address + ".";
                break;

            case NotificationType::DRIVER_ASSIGNED:
                title = "Driver Found!";
                body  = "Your driver " + ride.getDriver()->getName()
                      + " is heading to you. "
                      + ride.getDriver()->getVehicleModel()
                      + " | " + ride.getDriver()->getVehicleNumber();
                break;

            case NotificationType::DRIVER_ARRIVING:
                title = "Driver Arriving";
                body  = ride.getDriver()->getName()
                      + " is almost at your pickup. Please be ready!";
                break;

            case NotificationType::RIDE_STARTED:
                title = "Ride Started";
                body  = "You're on your way to " + ride.getDropoff().address
                      + ". Have a safe journey!";
                break;

            case NotificationType::RIDE_COMPLETED:
                title = "Ride Completed";
                body  = "You've arrived at " + ride.getDropoff().address
                      + ". Fare: " + formatFare(ride.getFare()) + ".";
                break;

            case NotificationType::PAYMENT_PROCESSED:
                title = "Payment Processed";
                body  = "Payment of " + formatFare(ride.getFare())
                      + " debited for ride #" + ride.getId() + ". Thank you!";
                break;

            case NotificationType::RIDE_CANCELLED:
                title = "Ride Cancelled";
                body  = "Your ride #" + ride.getId() + " has been cancelled.";
                break;

            default:
                title = "Ride Update";
                body  = "Update on your ride #" + ride.getId();
        }

        return std::make_unique<Notification>(
            nextId(), title, body, type, ride.getRider()->getId());
    }

    // Notification addressed to the driver
    static std::unique_ptr<Notification> createForDriver(
            const Ride& ride, NotificationType type) {

        if (!ride.getDriver()) return nullptr;

        std::string title, body;

        switch (type) {
            case NotificationType::RIDE_REQUESTED:
                title = "New Ride Request";
                body  = "Pickup: " + ride.getPickup().address
                      + " → Drop: " + ride.getDropoff().address;
                break;

            case NotificationType::RIDE_COMPLETED:
                title = "Trip Completed";
                body  = "Trip with " + ride.getRider()->getName()
                      + " completed. Earnings: " + formatFare(ride.getFare() * 0.8);
                break;

            case NotificationType::RIDE_CANCELLED:
                title = "Ride Cancelled";
                body  = ride.getRider()->getName()
                      + " cancelled the ride. You are now available for new rides.";
                break;

            default:
                title = "Ride Update";
                body  = "Update on ride #" + ride.getId();
        }

        return std::make_unique<Notification>(
            nextId(), title, body, type, ride.getDriver()->getId());
    }
};
