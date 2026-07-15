package models;

import enums.PaymentMethodType;

/**
 * Represents the user's Uber Wallet as a selectable payment method.
 * Acts as a thin wrapper allowing Wallet to go through the uniform PaymentStrategy interface.
 */
public class WalletPaymentMethod extends PaymentMethod {

    public WalletPaymentMethod(String methodId, String userId) {
        super(methodId, userId, PaymentMethodType.WALLET);
    }

    @Override
    public boolean isValid() {
        return true;  // Validity is determined by wallet balance at pay time
    }

    @Override
    public String toString() {
        return "WalletPaymentMethod{methodId='" + methodId + "', userId='" + userId + "'}";
    }
}
