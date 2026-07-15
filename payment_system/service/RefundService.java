package service;

import enums.PaymentStatus;
import models.Transaction;
import observer.PaymentEventListener;
import strategy.PaymentStrategy;

import java.util.List;

/**
 * Handles refund processing for failed or disputed rides.
 * Refunds are credited back to the original payment method or to the wallet.
 */
public class RefundService {
    private final TransactionService transactionService;
    private final List<PaymentEventListener> listeners;

    public RefundService(TransactionService transactionService, List<PaymentEventListener> listeners) {
        this.transactionService = transactionService;
        this.listeners = listeners;
    }

    /**
     * Initiates a full refund for a given transaction.
     */
    public Transaction initiateRefund(String originalTransactionId, PaymentStrategy strategy) {
        Transaction original = transactionService.getById(originalTransactionId);

        if (original == null) {
            throw new IllegalArgumentException("Transaction not found: " + originalTransactionId);
        }
        if (original.getStatus() != PaymentStatus.SUCCESS) {
            throw new IllegalStateException("Only successful transactions can be refunded. Status: " + original.getStatus());
        }
        if (original.getRefundTransactionId() != null) {
            throw new IllegalStateException("Transaction already refunded: " + original.getRefundTransactionId());
        }

        listeners.forEach(l -> l.onRefundInitiated(original));

        Transaction refundTxn = strategy.refund(original);
        transactionService.save(refundTxn);

        if (refundTxn.getStatus() == PaymentStatus.SUCCESS) {
            original.setStatus(PaymentStatus.REFUNDED);
            original.setRefundTransactionId(refundTxn.getTransactionId());
            listeners.forEach(l -> l.onRefundSuccess(refundTxn));
        }

        return refundTxn;
    }
}
