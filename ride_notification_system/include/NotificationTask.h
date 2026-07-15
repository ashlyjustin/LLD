#pragma once
#include <memory>
#include "User.h"
#include "Notification.h"

// A task unit that carries everything the consumer needs to deliver one notification.
struct NotificationTask {
    std::shared_ptr<User> recipient;
    Notification          notification;

    NotificationTask(std::shared_ptr<User> r, Notification n)
        : recipient(std::move(r)), notification(std::move(n)) {}
};
