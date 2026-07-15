#pragma once
#include <iostream>
#include "INotificationChannel.h"

class PushChannel : public INotificationChannel {
public:
    void send(const User& user, const Notification& notif) override {
        std::cout << "  [PUSH  → " << user.getName() << "] "
                  << notif.title << " — " << notif.body << "\n";
    }
    ChannelType getType() const override { return ChannelType::PUSH; }
};
