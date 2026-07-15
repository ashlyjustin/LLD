#pragma once
#include <iostream>
#include "IRideEventObserver.h"
#include "NotificationFactory.h"
#include "NotificationConsumer.h"
#include "NotificationTask.h"
#include "Ride.h"

/*
 * NotificationService — Observer + Publisher.
 *
 * Responsibilities:
 *   1. React to ride events (implements IRideEventObserver).
 *   2. Build Notification objects via NotificationFactory.
 *   3. Enqueue NotificationTasks into NotificationConsumer (non-blocking).
 *
 * NotificationService no longer owns channels or dispatches synchronously.
 * Channel selection and delivery happen asynchronously inside the consumer's
 * worker threads. A failed enqueue (consumer stopped) is logged and dropped —
 * it never rolls back a ride state transition.
 */
class NotificationService : public IRideEventObserver {
    NotificationConsumer& consumer;

    void sendToRider(const Ride& ride, NotificationType type) {
        auto notif = NotificationFactory::createForRider(ride, type);
        if (!notif) return;
        if (!consumer.enqueue({ride.getRider(), std::move(*notif)}))
            std::cerr << "  [WARN] NotificationService: consumer stopped, rider notification dropped.\n";
    }

    void sendToDriver(const Ride& ride, NotificationType type) {
        if (!ride.getDriver()) return;
        auto notif = NotificationFactory::createForDriver(ride, type);
        if (!notif) return;
        if (!consumer.enqueue({ride.getDriver(), std::move(*notif)}))
            std::cerr << "  [WARN] NotificationService: consumer stopped, driver notification dropped.\n";
    }

public:
    explicit NotificationService(NotificationConsumer& c) : consumer(c) {}

    // ── IRideEventObserver ──────────────────────────────────────────────────

    void onRideRequested(const Ride& ride) override {
        std::cout << "\n  [NotificationService] Ride requested → enqueuing rider notification\n";
        sendToRider(ride, NotificationType::RIDE_REQUESTED);
    }

    void onDriverAssigned(const Ride& ride) override {
        std::cout << "\n  [NotificationService] Driver assigned → enqueuing rider + driver notifications\n";
        sendToRider(ride, NotificationType::DRIVER_ASSIGNED);
        sendToDriver(ride, NotificationType::RIDE_REQUESTED);
    }

    void onDriverArriving(const Ride& ride) override {
        std::cout << "\n  [NotificationService] Driver arriving → enqueuing rider notification\n";
        sendToRider(ride, NotificationType::DRIVER_ARRIVING);
    }

    void onRideStarted(const Ride& ride) override {
        std::cout << "\n  [NotificationService] Ride started → enqueuing rider notification\n";
        sendToRider(ride, NotificationType::RIDE_STARTED);
    }

    void onRideCompleted(const Ride& ride) override {
        std::cout << "\n  [NotificationService] Ride completed → enqueuing rider + driver + payment notifications\n";
        sendToRider(ride, NotificationType::RIDE_COMPLETED);
        sendToDriver(ride, NotificationType::RIDE_COMPLETED);
        sendToRider(ride, NotificationType::PAYMENT_PROCESSED);
    }

    void onRideCancelled(const Ride& ride) override {
        std::cout << "\n  [NotificationService] Ride cancelled → enqueuing notifications\n";
        sendToRider(ride, NotificationType::RIDE_CANCELLED);
        sendToDriver(ride, NotificationType::RIDE_CANCELLED);
    }
};
