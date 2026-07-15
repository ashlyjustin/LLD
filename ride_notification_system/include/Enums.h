#pragma once
#include <string>

enum class RideStatus {
    REQUESTED,
    DRIVER_ASSIGNED,
    DRIVER_ARRIVING,
    IN_PROGRESS,
    COMPLETED,
    CANCELLED
};

enum class NotificationType {
    RIDE_REQUESTED,
    DRIVER_ASSIGNED,
    DRIVER_ARRIVING,
    RIDE_STARTED,
    RIDE_COMPLETED,
    RIDE_CANCELLED,
    PAYMENT_PROCESSED
};

enum class ChannelType {
    PUSH,
    SMS,
    EMAIL
};

inline std::string toString(RideStatus s) {
    switch (s) {
        case RideStatus::REQUESTED:       return "REQUESTED";
        case RideStatus::DRIVER_ASSIGNED: return "DRIVER_ASSIGNED";
        case RideStatus::DRIVER_ARRIVING: return "DRIVER_ARRIVING";
        case RideStatus::IN_PROGRESS:     return "IN_PROGRESS";
        case RideStatus::COMPLETED:       return "COMPLETED";
        case RideStatus::CANCELLED:       return "CANCELLED";
    }
    return "UNKNOWN";
}

inline std::string toString(NotificationType t) {
    switch (t) {
        case NotificationType::RIDE_REQUESTED:    return "RIDE_REQUESTED";
        case NotificationType::DRIVER_ASSIGNED:   return "DRIVER_ASSIGNED";
        case NotificationType::DRIVER_ARRIVING:   return "DRIVER_ARRIVING";
        case NotificationType::RIDE_STARTED:      return "RIDE_STARTED";
        case NotificationType::RIDE_COMPLETED:    return "RIDE_COMPLETED";
        case NotificationType::RIDE_CANCELLED:    return "RIDE_CANCELLED";
        case NotificationType::PAYMENT_PROCESSED: return "PAYMENT_PROCESSED";
    }
    return "UNKNOWN";
}

inline std::string toString(ChannelType c) {
    switch (c) {
        case ChannelType::PUSH:  return "PUSH";
        case ChannelType::SMS:   return "SMS";
        case ChannelType::EMAIL: return "EMAIL";
    }
    return "UNKNOWN";
}
