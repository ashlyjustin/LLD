import enums.Currency;
import models.*;
import observer.NotificationService;
import service.*;
import factory.PaymentStrategyFactory;

import java.time.YearMonth;

/**
 * Demonstrates key payment flows for the Uber Payment System LLD.
 *
 * Flows covered:
 *  1. Add payment methods (Credit Card, UPI, Wallet)
 *  2. Pay for a ride using Credit Card
 *  3. Pay using Wallet
 *  4. Pay using UPI
 *  5. Wallet top-up
 *  6. Refund a transaction
 *  7. Fallback to wallet on primary payment failure
 */
public class Main {

    public static void main(String[] args) {
        // ── Bootstrap services ────────────────────────────────────────────────
        TransactionService transactionService = new TransactionService();
        WalletService walletService = new WalletService();
        PaymentService paymentService = new PaymentService(transactionService);
        RefundService refundService = new RefundService(transactionService, paymentService.getListeners());

        // Register observers
        paymentService.registerListener(new NotificationService());

        // ── 1. Create a user ──────────────────────────────────────────────────
        User rider = new User("U001", "Alice", "alice@example.com", "+91-9876543210");
        System.out.println("\n=== Created " + rider + " ===");

        // ── 2. Add payment methods ────────────────────────────────────────────
        CreditCard creditCard = new CreditCard(
                "PM001", rider.getUserId(), "4321",
                "Alice", YearMonth.of(2027, 12), "hashed_cvv", "VISA"
        );
        creditCard.setDefault(true);
        rider.addPaymentMethod(creditCard);

        UPI upi = new UPI("PM002", rider.getUserId(), "alice@okicici", "ICICI-ACC-001");
        rider.addPaymentMethod(upi);

        System.out.println("Added payment methods: " + rider.getPaymentMethods());

        // ── 3. Create a wallet and top it up ─────────────────────────────────
        System.out.println("\n=== Wallet Operations ===");
        walletService.createWallet(rider, Currency.INR);
        walletService.topUp(rider, 500.0);
        System.out.println("Wallet balance: ₹" + walletService.getBalance(rider));

        // ── 4. Pay for a ride using Credit Card (default) ────────────────────
        System.out.println("\n=== Ride 1: Pay with Credit Card ===");
        Ride ride1 = new Ride("R001", rider.getUserId(), "D001", 250.0, Currency.INR);
        Transaction txn1 = paymentService.processPaymentWithDefault(rider, ride1);
        System.out.println("Result: " + txn1);

        // Generate invoice
        Invoice invoice1 = paymentService.generateInvoice(ride1, txn1, 1.2, 30.0);
        System.out.println("Invoice: " + invoice1);

        // ── 5. Pay for a ride using UPI ───────────────────────────────────────
        System.out.println("\n=== Ride 2: Pay with UPI ===");
        Ride ride2 = new Ride("R002", rider.getUserId(), "D002", 180.0, Currency.INR);
        Transaction txn2 = paymentService.processPayment(rider, ride2, upi);
        System.out.println("Result: " + txn2);

        // ── 6. Pay for a ride using Wallet ────────────────────────────────────
        System.out.println("\n=== Ride 3: Pay with Wallet ===");
        WalletPaymentMethod walletMethod = new WalletPaymentMethod("PM003", rider.getUserId());
        rider.addPaymentMethod(walletMethod);
        Ride ride3 = new Ride("R003", rider.getUserId(), "D003", 120.0, Currency.INR);
        Transaction txn3 = paymentService.processPayment(rider, ride3, walletMethod);
        System.out.println("Result: " + txn3);
        System.out.println("Wallet balance after payment: ₹" + walletService.getBalance(rider));

        // ── 7. Refund ride 1 ─────────────────────────────────────────────────
        System.out.println("\n=== Refund: Ride 1 Transaction ===");
        Transaction refundTxn = refundService.initiateRefund(
                txn1.getTransactionId(),
                PaymentStrategyFactory.getStrategy(txn1.getPaymentMethodType(), rider)
        );
        System.out.println("Refund result: " + refundTxn);
        System.out.println("Original txn status after refund: " + txn1.getStatus());

        // ── 8. Transaction history ────────────────────────────────────────────
        System.out.println("\n=== Transaction History for " + rider.getName() + " ===");
        transactionService.getByUserId(rider.getUserId())
                .forEach(System.out::println);
    }
}
