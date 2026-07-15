package strategy;

import enums.Currency;
import enums.PaymentMethodType;
import enums.PaymentStatus;
import enums.TransactionType;
import models.DebitCard;
import models.PaymentMethod;
import models.Transaction;

import java.util.UUID;

public class DebitCardPaymentStrategy implements PaymentStrategy {

    @Override
    public Transaction pay(String userId, String rideId, double amount, PaymentMethod paymentMethod) {
        DebitCard card = (DebitCard) paymentMethod;
        Transaction txn = new Transaction(
                UUID.randomUUID().toString(), userId, rideId,
                amount, Currency.INR,
                PaymentMethodType.DEBIT_CARD, TransactionType.DEBIT
        );

        if (!card.isValid()) {
            txn.setStatus(PaymentStatus.FAILED);
            txn.setFailureReason("Card expired");
            return txn;
        }

        txn.setStatus(PaymentStatus.PROCESSING);
        boolean success = processWithBank(card, amount);

        if (success) {
            txn.setStatus(PaymentStatus.SUCCESS);
        } else {
            txn.setStatus(PaymentStatus.FAILED);
            txn.setFailureReason("Insufficient funds");
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
                PaymentMethodType.DEBIT_CARD, TransactionType.REFUND
        );
        refundTxn.setStatus(PaymentStatus.SUCCESS);
        return refundTxn;
    }

    private boolean processWithBank(DebitCard card, double amount) {
        System.out.println("[BankGateway] Charging " + amount + " on debit card ending " + card.getCardNumber());
        return true;
    }
}
