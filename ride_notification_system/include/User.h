#pragma once
#include <string>
#include <vector>
#include "Enums.h"

// Represents a rider or any app user with notification channel preferences.
class User {
protected:
    std::string id;
    std::string name;
    std::string phone;
    std::string email;
    std::vector<ChannelType> preferredChannels;

public:
    User(std::string id, std::string name, std::string phone, std::string email,
         std::vector<ChannelType> channels = {ChannelType::PUSH})
        : id(std::move(id))
        , name(std::move(name))
        , phone(std::move(phone))
        , email(std::move(email))
        , preferredChannels(std::move(channels)) {}

    virtual ~User() = default;

    const std::string& getId()    const { return id; }
    const std::string& getName()  const { return name; }
    const std::string& getPhone() const { return phone; }
    const std::string& getEmail() const { return email; }
    const std::vector<ChannelType>& getPreferredChannels() const { return preferredChannels; }

    void addChannel(ChannelType ch) { preferredChannels.push_back(ch); }
};
