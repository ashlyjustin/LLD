#pragma once
#include "../models/Trip.h"

// Observer interface — notified on every trip status change.
class TripObserver {
public:
    virtual ~TripObserver() = default;
    virtual void onTripStatusChanged(const Trip& trip) = 0;
};
