package models;

import enums.Currency;

import java.time.LocalDateTime;

public class Ride {
    private final String rideId;
    private final String riderId;
    private final String driverId;
    private double fare;
    private final Currency currency;
    private final LocalDateTime startTime;
    private LocalDateTime endTime;
    private boolean isPaid;

    public Ride(String rideId, String riderId, String driverId, double fare, Currency currency) {
        this.rideId = rideId;
        this.riderId = riderId;
        this.driverId = driverId;
        this.fare = fare;
        this.currency = currency;
        this.startTime = LocalDateTime.now();
        this.isPaid = false;
    }

    public String getRideId() { return rideId; }
    public String getRiderId() { return riderId; }
    public String getDriverId() { return driverId; }
    public double getFare() { return fare; }
    public Currency getCurrency() { return currency; }
    public LocalDateTime getStartTime() { return startTime; }
    public LocalDateTime getEndTime() { return endTime; }
    public boolean isPaid() { return isPaid; }

    public void setFare(double fare) { this.fare = fare; }
    public void setEndTime(LocalDateTime endTime) { this.endTime = endTime; }
    public void markPaid() { this.isPaid = true; }

    @Override
    public String toString() {
        return "Ride{rideId='" + rideId + "', fare=" + fare + " " + currency + "', paid=" + isPaid + "}";
    }
}
