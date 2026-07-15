package strategy;

import enums.Currency;
import enums.PaymentMethodType;
import enums.PaymentStatus;
import enums.TransactionType;
import models.PaymentMethod;
import models.Transaction;
import models.User;
import models.Wallet;

import java.util.UUID;

public class WalletPaymentStrategy implements PaymentStrategy {
    private final User user;

    public WalletPaymentStrategy(User user) {
        this.user = user;
    }

    @Override
    public Transaction pay(String userId, String rideId, double amount, PaymentMethod paymentMethod) {
        Wallet wallet = user.getWallet();
        Transaction txn = new Transaction(
                UUID.randomUUID().toString(), userId, rideId,
                amount, Currency.INR,
                PaymentMethodType.WALLET, TransactionType.DEBIT
        );

        if (wallet == null) {
            txn.setStatus(PaymentStatus.FAILED);
            txn.setFailureReason("No wallet linked");
            return txn;
        }

        txn.setStatus(PaymentStatus.PROCESSING);
        boolean success = wallet.debit(amount);

        if (success) {
            txn.setStatus(PaymentStatus.SUCCESS);
            System.out.println("[Wallet] Debited " + amount + ". Remaining balance: " + wallet.getBalance());
        } else {
            txn.setStatus(PaymentStatus.FAILED);
            txn.setFailureReason("Insufficient wallet balance");
        }
        return txn;
    }

    @Override
    public Transaction refund(Transaction originalTransaction) {
        Wallet wallet = user.getWallet();
        Transaction refundTxn = new Transaction(
                UUID.randomUUID().toString(),
                originalTransaction.getUserId(),
                originalTransaction.getRideId(),
                originalTransaction.getAmount(),
                originalTransaction.getCurrency(),
                PaymentMethodType.WALLET, TransactionType.REFUND
        );

        if (wallet != null) {
            wallet.credit(originalTransaction.getAmount());
            refundTxn.setStatus(PaymentStatus.SUCCESS);
            System.out.println("[Wallet] Refunded " + originalTransaction.getAmount() + ". New balance: " + wallet.getBalance());
        } else {
            refundTxn.setStatus(PaymentStatus.FAILED);
            refundTxn.setFailureReason("No wallet linked for refund");
        }
        return refundTxn;
    }
}
