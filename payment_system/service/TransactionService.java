package service;

import models.Transaction;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Manages persistence and retrieval of all transactions.
 * In production, this would delegate to a database repository.
 */
public class TransactionService {
    private final Map<String, Transaction> transactionStore = new HashMap<>();
    private final Map<String, List<Transaction>> userTransactions = new HashMap<>();

    public void save(Transaction transaction) {
        transactionStore.put(transaction.getTransactionId(), transaction);
        userTransactions
                .computeIfAbsent(transaction.getUserId(), k -> new ArrayList<>())
                .add(transaction);
    }

    public Transaction getById(String transactionId) {
        return transactionStore.get(transactionId);
    }

    public List<Transaction> getByUserId(String userId) {
        return userTransactions.getOrDefault(userId, new ArrayList<>());
    }

    public List<Transaction> getByRideId(String rideId) {
        return transactionStore.values().stream()
                .filter(t -> t.getRideId().equals(rideId))
                .toList();
    }
}
