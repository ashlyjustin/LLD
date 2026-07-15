package models;

import enums.Currency;

import java.util.concurrent.locks.ReentrantLock;

public class Wallet {
    private final String walletId;
    private final String userId;
    private double balance;
    private final Currency currency;
    private final ReentrantLock lock = new ReentrantLock();

    public Wallet(String walletId, String userId, double initialBalance, Currency currency) {
        this.walletId = walletId;
        this.userId = userId;
        this.balance = initialBalance;
        this.currency = currency;
    }

    public String getWalletId() { return walletId; }
    public String getUserId() { return userId; }
    public Currency getCurrency() { return currency; }

    public double getBalance() {
        lock.lock();
        try {
            return balance;
        } finally {
            lock.unlock();
        }
    }

    public boolean debit(double amount) {
        lock.lock();
        try {
            if (balance < amount) return false;
            balance -= amount;
            return true;
        } finally {
            lock.unlock();
        }
    }

    public void credit(double amount) {
        lock.lock();
        try {
            balance += amount;
        } finally {
            lock.unlock();
        }
    }

    @Override
    public String toString() {
        return "Wallet{walletId='" + walletId + "', balance=" + balance + " " + currency + "}";
    }
}
