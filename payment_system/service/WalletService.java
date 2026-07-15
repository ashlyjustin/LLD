package service;

import enums.Currency;
import models.User;
import models.Wallet;

import java.util.UUID;

/**
 * Manages wallet operations: creation and top-up.
 */
public class WalletService {

    public Wallet createWallet(User user, Currency currency) {
        if (user.getWallet() != null) {
            throw new IllegalStateException("User already has a wallet");
        }
        Wallet wallet = new Wallet(UUID.randomUUID().toString(), user.getUserId(), 0.0, currency);
        user.setWallet(wallet);
        System.out.println("[WalletService] Created wallet for user " + user.getName() +
                " | WalletId: " + wallet.getWalletId());
        return wallet;
    }

    public void topUp(User user, double amount) {
        Wallet wallet = user.getWallet();
        if (wallet == null) {
            throw new IllegalStateException("User has no wallet. Please create one first.");
        }
        if (amount <= 0) {
            throw new IllegalArgumentException("Top-up amount must be positive");
        }
        wallet.credit(amount);
        System.out.println("[WalletService] Topped up " + amount + " " + wallet.getCurrency() +
                ". New balance: " + wallet.getBalance());
    }

    public double getBalance(User user) {
        if (user.getWallet() == null) {
            throw new IllegalStateException("User has no wallet");
        }
        return user.getWallet().getBalance();
    }
}
