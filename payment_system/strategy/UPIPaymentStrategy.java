package strategy;

import enums.Currency;
import enums.PaymentMethodType;
import enums.PaymentStatus;
import enums.TransactionType;
import models.PaymentMethod;
import models.Transaction;
import models.UPI;

import java.util.UUID;

public class UPIPaymentStrategy implements PaymentStrategy {

    @Override
    public Transaction pay(String userId, String rideId, double amount, PaymentMethod paymentMethod) {
        UPI upi = (UPI) paymentMethod;
        Transaction txn = new Transaction(
                UUID.randomUUID().toString(), userId, rideId,
                amount, Currency.INR,
                PaymentMethodType.UPI, TransactionType.DEBIT
        );

        if (!upi.isValid()) {
            txn.setStatus(PaymentStatus.FAILED);
            txn.setFailureReason("Invalid UPI ID");
            return txn;
        }

        txn.setStatus(PaymentStatus.PROCESSING);
        boolean success = processUPIPayment(upi, amount);

        if (success) {
            txn.setStatus(PaymentStatus.SUCCESS);
        } else {
            txn.setStatus(PaymentStatus.FAILED);
            txn.setFailureReason("UPI transaction timed out");
        }
        return txn;
    }

    @Override
    public Transaction refund(Transaction originalTransaction) {
        Transaction refundTxn = new Transaction(
                UUID.randomUUID().toString(),
                originalTransaction.getUserId(),
                originalTransaction.getRideId(),
                originalTransaction.getAmount(),
                originalTransaction.getCurrency(),
                PaymentMethodType.UPI, TransactionType.REFUND
        );
        refundTxn.setStatus(PaymentStatus.SUCCESS);
        return refundTxn;
    }

    private boolean processUPIPayment(UPI upi, double amount) {
        System.out.println("[UPIGateway] Processing " + amount + " via " + upi.getUpiId());
        return true;
    }
}
