package observer;

import models.Transaction;

/**
 * Concrete observer: sends notifications (email/SMS/push) to users on payment events.
 */
public class NotificationService implements PaymentEventListener {

    @Override
    public void onPaymentSuccess(Transaction transaction) {
        System.out.println("[Notification] Payment of " + transaction.getAmount() +
                " " + transaction.getCurrency() +
                " successful for ride " + transaction.getRideId() +
                " | TxnId: " + transaction.getTransactionId());
    }

    @Override
    public void onPaymentFailure(Transaction transaction) {
        System.out.println("[Notification] Payment FAILED for ride " + transaction.getRideId() +
                " | Reason: " + transaction.getFailureReason() +
                " | TxnId: " + transaction.getTransactionId());
    }

    @Override
    public void onRefundInitiated(Transaction refundTransaction) {
        System.out.println("[Notification] Refund initiated for TxnId: " + refundTransaction.getTransactionId());
    }

    @Override
    public void onRefundSuccess(Transaction refundTransaction) {
        System.out.println("[Notification] Refund of " + refundTransaction.getAmount() +
                " " + refundTransaction.getCurrency() + " successful" +
                " | RefundTxnId: " + refundTransaction.getTransactionId());
    }
}
