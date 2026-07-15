package models;

import enums.PaymentMethodType;

public class UPI extends PaymentMethod {
    private final String upiId;  // e.g., user@okicici
    private final String linkedBankAccount;

    public UPI(String methodId, String userId, String upiId, String linkedBankAccount) {
        super(methodId, userId, PaymentMethodType.UPI);
        this.upiId = upiId;
        this.linkedBankAccount = linkedBankAccount;
    }

    public String getUpiId() { return upiId; }
    public String getLinkedBankAccount() { return linkedBankAccount; }

    @Override
    public boolean isValid() {
        // Simple format check: something@something
        return upiId != null && upiId.contains("@");
    }

    @Override
    public String toString() {
        return "UPI{upiId='" + upiId + "', valid=" + isValid() + "}";
    }
}
