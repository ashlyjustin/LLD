#pragma once
#include <string>
#include <chrono>
#include "Enums.h"

class Notification {
public:
    std::string id;
    std::string title;
    std::string body;
    NotificationType type;
    std::string recipientId;
    std::chrono::system_clock::time_point timestamp;

    Notification(std::string id, std::string title, std::string body,
                 NotificationType type, std::string recipientId)
        : id(std::move(id))
        , title(std::move(title))
        , body(std::move(body))
        , type(type)
        , recipientId(std::move(recipientId))
        , timestamp(std::chrono::system_clock::now()) {}
};
