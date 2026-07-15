#pragma once

// Forward declaration to break include cycle: Ride ↔ Observer
class Ride;

/*
 * Observer interface for ride lifecycle events.
 * RideService (Subject) notifies all registered observers.
 *
 * Extension point: add AnalyticsObserver, SurgePricingObserver, etc.
 * without touching RideService.
 */
class IRideEventObserver {
public:
    virtual ~IRideEventObserver() = default;

    virtual void onRideRequested(const Ride& ride)   = 0;
    virtual void onDriverAssigned(const Ride& ride)  = 0;
    virtual void onDriverArriving(const Ride& ride)  = 0;
    virtual void onRideStarted(const Ride& ride)     = 0;
    virtual void onRideCompleted(const Ride& ride)   = 0;
    virtual void onRideCancelled(const Ride& ride)   = 0;
};
