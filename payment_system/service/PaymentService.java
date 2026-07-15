package service;

import enums.PaymentMethodType;
import enums.PaymentStatus;
import factory.PaymentStrategyFactory;
import models.*;
import observer.PaymentEventListener;
import strategy.PaymentStrategy;

import java.util.ArrayList;
import java.util.List;

/**
 * Core orchestrator for payment flows.
 * Coordinates strategy selection, transaction management, and event notifications.
 */
public class PaymentService {
    private final TransactionService transactionService;
    private final List<PaymentEventListener> listeners = new ArrayList<>();

    public PaymentService(TransactionService transactionService) {
        this.transactionService = transactionService;
    }

    public void registerListener(PaymentEventListener listener) {
        listeners.add(listener);
    }

    /**
     * Processes payment for a completed ride using the user's chosen payment method.
     */
    public Transaction processPayment(User user, Ride ride, PaymentMethod paymentMethod) {
        if (ride.isPaid()) {
            throw new IllegalStateException("Ride " + ride.getRideId() + " is already paid");
        }
        if (!paymentMethod.isValid()) {
            throw new IllegalArgumentException("Invalid payment method: " + paymentMethod.getMethodId());
        }

        PaymentStrategy strategy = PaymentStrategyFactory.getStrategy(paymentMethod.getType(), user);
        Transaction txn = strategy.pay(user.getUserId(), ride.getRideId(), ride.getFare(), paymentMethod);

        transactionService.save(txn);

        if (txn.getStatus() == PaymentStatus.SUCCESS) {
            ride.markPaid();
            listeners.forEach(l -> l.onPaymentSuccess(txn));
        } else {
            listeners.forEach(l -> l.onPaymentFailure(txn));
        }

        return txn;
    }

    /**
     * Processes payment using the user's default payment method.
     */
    public Transaction processPaymentWithDefault(User user, Ride ride) {
        PaymentMethod defaultMethod = user.getPaymentMethods().stream()
                .filter(PaymentMethod::isDefault)
                .findFirst()
                .orElseThrow(() -> new IllegalStateException("No default payment method set for user: " + user.getUserId()));

        return processPayment(user, ride, defaultMethod);
    }

    /**
     * Processes payment with fallback: tries the given method; on failure, tries the wallet.
     */
    public Transaction processPaymentWithFallback(User user, Ride ride, PaymentMethod primaryMethod) {
        Transaction txn = processPayment(user, ride, primaryMethod);

        if (txn.getStatus() != PaymentStatus.SUCCESS && user.getWallet() != null
                && user.getWallet().getBalance() >= ride.getFare()) {
            System.out.println("[PaymentService] Primary payment failed. Falling back to wallet...");
            PaymentStrategy walletStrategy = PaymentStrategyFactory.getStrategy(PaymentMethodType.WALLET, user);
            txn = walletStrategy.pay(user.getUserId(), ride.getRideId(), ride.getFare(), null);
            transactionService.save(txn);

            if (txn.getStatus() == PaymentStatus.SUCCESS) {
                ride.markPaid();
                listeners.forEach(l -> l.onPaymentSuccess(txn));
            } else {
                listeners.forEach(l -> l.onPaymentFailure(txn));
            }
        }
        return txn;
    }

    /**
     * Generates an invoice for a successfully paid ride.
     */
    public Invoice generateInvoice(Ride ride, Transaction transaction, double surgeMultiplier, double taxes) {
        if (transaction.getStatus() != PaymentStatus.SUCCESS) {
            throw new IllegalStateException("Cannot generate invoice for unsuccessful payment");
        }
        return new Invoice(
                "INV-" + transaction.getTransactionId(),
                ride.getRideId(),
                transaction.getUserId(),
                ride.getFare() / surgeMultiplier - taxes / surgeMultiplier,  // back-calculate base
                surgeMultiplier,
                taxes,
                transaction.getCurrency(),
                transaction.getTransactionId()
        );
    }

    public List<PaymentEventListener> getListeners() {
        return listeners;
    }
}
