#pragma once
#include "User.h"
#include "Notification.h"

/*
 * Strategy interface for notification delivery channels.
 * Concrete strategies: PushChannel, SMSChannel, EmailChannel.
 */
class INotificationChannel {
public:
    virtual ~INotificationChannel() = default;
    virtual void send(const User& user, const Notification& notification) = 0;
    virtual ChannelType getType() const = 0;
};
