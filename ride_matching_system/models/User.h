#pragma once
#include <string>

class User {
public:
    std::string id;
    std::string name;
    std::string phone;

    User(std::string id, std::string name, std::string phone)
        : id(std::move(id)), name(std::move(name)), phone(std::move(phone)) {}

    virtual ~User() = default;

    void addRating(double rating) {
        ratingSum += rating;
        ++ratingCount;
    }

    double getRating()     const { return ratingCount > 0 ? ratingSum / ratingCount : 5.0; }
    int    getRatingCount() const { return ratingCount; }

private:
    double ratingSum   = 0.0;
    int    ratingCount = 0;
};
