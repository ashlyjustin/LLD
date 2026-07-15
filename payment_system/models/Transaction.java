package models;

import enums.Currency;
import enums.PaymentMethodType;
import enums.PaymentStatus;
import enums.TransactionType;

import java.time.LocalDateTime;

public class Transaction {
    private final String transactionId;
    private final String userId;
    private final String rideId;
    private final double amount;
    private final Currency currency;
    private final PaymentMethodType paymentMethodType;
    private final TransactionType transactionType;
    private PaymentStatus status;
    private final LocalDateTime createdAt;
    private LocalDateTime updatedAt;
    private String failureReason;
    private String refundTransactionId;  // set if this txn was refunded

    public Transaction(String transactionId, String userId, String rideId,
                       double amount, Currency currency,
                       PaymentMethodType paymentMethodType, TransactionType transactionType) {
        this.transactionId = transactionId;
        this.userId = userId;
        this.rideId = rideId;
        this.amount = amount;
        this.currency = currency;
        this.paymentMethodType = paymentMethodType;
        this.transactionType = transactionType;
        this.status = PaymentStatus.PENDING;
        this.createdAt = LocalDateTime.now();
        this.updatedAt = LocalDateTime.now();
    }

    public String getTransactionId() { return transactionId; }
    public String getUserId() { return userId; }
    public String getRideId() { return rideId; }
    public double getAmount() { return amount; }
    public Currency getCurrency() { return currency; }
    public PaymentMethodType getPaymentMethodType() { return paymentMethodType; }
    public TransactionType getTransactionType() { return transactionType; }
    public PaymentStatus getStatus() { return status; }
    public LocalDateTime getCreatedAt() { return createdAt; }
    public LocalDateTime getUpdatedAt() { return updatedAt; }
    public String getFailureReason() { return failureReason; }
    public String getRefundTransactionId() { return refundTransactionId; }

    public void setStatus(PaymentStatus status) {
        this.status = status;
        this.updatedAt = LocalDateTime.now();
    }

    public void setFailureReason(String failureReason) {
        this.failureReason = failureReason;
    }

    public void setRefundTransactionId(String refundTransactionId) {
        this.refundTransactionId = refundTransactionId;
    }

    @Override
    public String toString() {
        return "Transaction{id='" + transactionId +
               "', userId='" + userId +
               "', rideId='" + rideId +
               "', amount=" + amount + " " + currency +
               "', type=" + transactionType +
               "', status=" + status + "}";
    }
}
