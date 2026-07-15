package strategy;

import enums.Currency;
import enums.PaymentMethodType;
import enums.PaymentStatus;
import enums.TransactionType;
import models.CreditCard;
import models.PaymentMethod;
import models.Transaction;

import java.util.UUID;

public class CreditCardPaymentStrategy implements PaymentStrategy {

    @Override
    public Transaction pay(String userId, String rideId, double amount, PaymentMethod paymentMethod) {
        CreditCard card = (CreditCard) paymentMethod;
        Transaction txn = new Transaction(
                UUID.randomUUID().toString(), userId, rideId,
                amount, Currency.INR,
                PaymentMethodType.CREDIT_CARD, TransactionType.DEBIT
        );

        if (!card.isValid()) {
            txn.setStatus(PaymentStatus.FAILED);
            txn.setFailureReason("Card expired");
            return txn;
        }

        txn.setStatus(PaymentStatus.PROCESSING);
        boolean success = processWithBank(card, amount);  // Simulates bank API call

        if (success) {
            txn.setStatus(PaymentStatus.SUCCESS);
        } else {
            txn.setStatus(PaymentStatus.FAILED);
            txn.setFailureReason("Bank declined the transaction");
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
                PaymentMethodType.CREDIT_CARD, TransactionType.REFUND
        );
        refundTxn.setStatus(PaymentStatus.SUCCESS);
        return refundTxn;
    }

    private boolean processWithBank(CreditCard card, double amount) {
        // Simulate bank gateway call — always succeeds in this demo
        System.out.println("[BankGateway] Charging " + amount + " on card ending " + card.getCardNumber());
        return true;
    }
}
