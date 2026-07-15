package strategy;

import models.PaymentMethod;
import models.Transaction;

/**
 * Strategy interface for processing payments.
 * Each payment method (CreditCard, DebitCard, UPI, Wallet) has its own implementation.
 */
public interface PaymentStrategy {
    Transaction pay(String userId, String rideId, double amount, PaymentMethod paymentMethod);
    Transaction refund(Transaction originalTransaction);
}
