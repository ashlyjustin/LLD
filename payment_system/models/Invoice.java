package models;

import enums.Currency;

import java.time.LocalDateTime;

public class Invoice {
    private final String invoiceId;
    private final String rideId;
    private final String userId;
    private final double baseFare;
    private final double surgeMultiplier;
    private final double taxes;
    private final double totalAmount;
    private final Currency currency;
    private final String transactionId;
    private final LocalDateTime generatedAt;

    public Invoice(String invoiceId, String rideId, String userId,
                   double baseFare, double surgeMultiplier, double taxes,
                   Currency currency, String transactionId) {
        this.invoiceId = invoiceId;
        this.rideId = rideId;
        this.userId = userId;
        this.baseFare = baseFare;
        this.surgeMultiplier = surgeMultiplier;
        this.taxes = taxes;
        this.totalAmount = (baseFare * surgeMultiplier) + taxes;
        this.currency = currency;
        this.transactionId = transactionId;
        this.generatedAt = LocalDateTime.now();
    }

    public String getInvoiceId() { return invoiceId; }
    public String getRideId() { return rideId; }
    public String getUserId() { return userId; }
    public double getBaseFare() { return baseFare; }
    public double getSurgeMultiplier() { return surgeMultiplier; }
    public double getTaxes() { return taxes; }
    public double getTotalAmount() { return totalAmount; }
    public Currency getCurrency() { return currency; }
    public String getTransactionId() { return transactionId; }
    public LocalDateTime getGeneratedAt() { return generatedAt; }

    @Override
    public String toString() {
        return "Invoice{invoiceId='" + invoiceId +
               "', rideId='" + rideId +
               "', baseFare=" + baseFare +
               ", surge=" + surgeMultiplier +
               "x, taxes=" + taxes +
               ", total=" + totalAmount + " " + currency + "}";
    }
}
