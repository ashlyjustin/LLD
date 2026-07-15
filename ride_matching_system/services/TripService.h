#pragma once
#include "../models/Trip.h"
#include "../observers/TripObserver.h"
#include "DriverService.h"
#include "RiderService.h"
#include <unordered_map>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <stdexcept>
#include <atomic>

// TripService is the single owner of trip creation and all state transitions.
// It enforces a strict state machine and notifies registered observers on every change.
class TripService {
public:
    TripService(DriverService& driverService, RiderService& riderService)
        : driverService_(driverService), riderService_(riderService) {}

    void addObserver(TripObserver* observer) {
        observers_.push_back(observer);
    }

    // ── Trip lifecycle ────────────────────────────────────────────────

    std::shared_ptr<Trip> createTrip(const std::string& riderId,
                                     const Location&    pickup,
                                     const Location&    drop,
                                     VehicleType        vehicleType) {
        static std::atomic<int> counter{0};
        auto trip = std::make_shared<Trip>(
            "TRIP_" + std::to_string(++counter),
            riderId, pickup, drop, vehicleType);

        trips_[trip->id] = trip;
        riderHistory_[riderId].push_back(trip->id);
        notify(*trip);
        return trip;
    }

    // REQUESTED → DRIVER_ASSIGNED
    void assignDriver(const std::string& tripId, const std::string& driverId) {
        auto trip = getTrip(tripId);
        validateTransition(trip->status, TripStatus::DRIVER_ASSIGNED);
        trip->driverId = driverId;
        trip->status   = TripStatus::DRIVER_ASSIGNED;
        driverService_.setAvailability(driverId, false);
        driverHistory_[driverId].push_back(tripId);
        notify(*trip);
    }

    // DRIVER_ASSIGNED → IN_PROGRESS
    void startTrip(const std::string& tripId) {
        auto trip = getTrip(tripId);
        validateTransition(trip->status, TripStatus::IN_PROGRESS);
        trip->status    = TripStatus::IN_PROGRESS;
        trip->startedAt = std::time(nullptr);
        notify(*trip);
    }

    // IN_PROGRESS → COMPLETED
    void completeTrip(const std::string& tripId,
                      double fare, double distanceKm, double durationMin) {
        auto trip = getTrip(tripId);
        validateTransition(trip->status, TripStatus::COMPLETED);
        trip->status      = TripStatus::COMPLETED;
        trip->fare        = fare;
        trip->distanceKm  = distanceKm;
        trip->durationMin = durationMin;
        trip->completedAt = std::time(nullptr);
        driverService_.setAvailability(trip->driverId, true);
        notify(*trip);
    }

    // REQUESTED or DRIVER_ASSIGNED → CANCELLED
    void cancelTrip(const std::string& tripId) {
        auto trip = getTrip(tripId);
        validateTransition(trip->status, TripStatus::CANCELLED);
        TripStatus prev = trip->status;
        trip->status    = TripStatus::CANCELLED;
        if (prev == TripStatus::DRIVER_ASSIGNED && !trip->driverId.empty())
            driverService_.setAvailability(trip->driverId, true);
        notify(*trip);
    }

    // ── Ratings ───────────────────────────────────────────────────────

    void rateDriver(const std::string& tripId, double rating) {
        auto trip = requireCompleted(tripId);
        if (trip->driverRated) throw std::runtime_error("Driver already rated for trip " + tripId);
        validateRating(rating);
        driverService_.getDriver(trip->driverId)->addRating(rating);
        trip->driverRated = true;
    }

    void rateRider(const std::string& tripId, double rating) {
        auto trip = requireCompleted(tripId);
        if (trip->riderRated) throw std::runtime_error("Rider already rated for trip " + tripId);
        validateRating(rating);
        riderService_.getRider(trip->riderId)->addRating(rating);
        trip->riderRated = true;
    }

    // ── Queries ───────────────────────────────────────────────────────

    std::shared_ptr<Trip> getTrip(const std::string& tripId) const {
        auto it = trips_.find(tripId);
        if (it == trips_.end()) throw std::runtime_error("Trip not found: " + tripId);
        return it->second;
    }

    std::vector<std::shared_ptr<Trip>> getRiderTripHistory(const std::string& riderId) const {
        return resolveHistory(riderHistory_, riderId);
    }

    std::vector<std::shared_ptr<Trip>> getDriverTripHistory(const std::string& driverId) const {
        return resolveHistory(driverHistory_, driverId);
    }

private:
    DriverService& driverService_;
    RiderService&  riderService_;
    std::unordered_map<std::string, std::shared_ptr<Trip>> trips_;
    std::vector<TripObserver*> observers_;
    std::unordered_map<std::string, std::vector<std::string>> riderHistory_;
    std::unordered_map<std::string, std::vector<std::string>> driverHistory_;

    // Valid state transitions
    static const std::map<TripStatus, std::set<TripStatus>>& transitions() {
        static const std::map<TripStatus, std::set<TripStatus>> t = {
            {TripStatus::REQUESTED,       {TripStatus::DRIVER_ASSIGNED, TripStatus::CANCELLED}},
            {TripStatus::DRIVER_ASSIGNED, {TripStatus::IN_PROGRESS,     TripStatus::CANCELLED}},
            {TripStatus::IN_PROGRESS,     {TripStatus::COMPLETED}},
            {TripStatus::COMPLETED,       {}},
            {TripStatus::CANCELLED,       {}},
        };
        return t;
    }

    void validateTransition(TripStatus from, TripStatus to) const {
        const auto& t = transitions();
        auto it = t.find(from);
        if (it == t.end() || it->second.find(to) == it->second.end())
            throw std::runtime_error(
                "Invalid transition: " + tripStatusToString(from) +
                " → " + tripStatusToString(to));
    }

    void validateRating(double rating) const {
        if (rating < 1.0 || rating > 5.0)
            throw std::invalid_argument("Rating must be between 1.0 and 5.0");
    }

    std::shared_ptr<Trip> requireCompleted(const std::string& tripId) const {
        auto trip = getTrip(tripId);
        if (trip->status != TripStatus::COMPLETED)
            throw std::runtime_error("Trip " + tripId + " is not yet completed");
        return trip;
    }

    std::vector<std::shared_ptr<Trip>> resolveHistory(
        const std::unordered_map<std::string, std::vector<std::string>>& index,
        const std::string& userId) const {
        std::vector<std::shared_ptr<Trip>> result;
        auto it = index.find(userId);
        if (it != index.end())
            for (const auto& tid : it->second)
                result.push_back(trips_.at(tid));
        return result;
    }

    void notify(const Trip& trip) {
        for (auto* obs : observers_) obs->onTripStatusChanged(trip);
    }
};
