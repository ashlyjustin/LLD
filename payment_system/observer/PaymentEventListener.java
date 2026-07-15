package observer;

import models.Transaction;

/**
 * Observer interface for payment lifecycle events.
 */
public interface PaymentEventListener {
    void onPaymentSuccess(Transaction transaction);
    void onPaymentFailure(Transaction transaction);
    void onRefundInitiated(Transaction refundTransaction);
    void onRefundSuccess(Transaction refundTransaction);
}
