#pragma once
#include <iostream>
#include "INotificationChannel.h"

class SMSChannel : public INotificationChannel {
public:
    void send(const User& user, const Notification& notif) override {
        std::cout << "  [SMS   → " << user.getPhone() << " (" << user.getName() << ")] "
                  << notif.body << "\n";
    }
    ChannelType getType() const override { return ChannelType::SMS; }
};
