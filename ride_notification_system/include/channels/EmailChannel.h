#pragma once
#include <iostream>
#include "INotificationChannel.h"

class EmailChannel : public INotificationChannel {
public:
    void send(const User& user, const Notification& notif) override {
        std::cout << "  [EMAIL → " << user.getEmail() << " (" << user.getName() << ")] "
                  << "Subject: " << notif.title << " | " << notif.body << "\n";
    }
    ChannelType getType() const override { return ChannelType::EMAIL; }
};
