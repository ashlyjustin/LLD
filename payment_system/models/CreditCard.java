package models;

import enums.PaymentMethodType;
import java.time.YearMonth;

public class CreditCard extends PaymentMethod {
    private final String cardNumber;       // masked, last 4 digits
    private final String cardHolderName;
    private final YearMonth expiryDate;
    private final String cvvHash;          // never store plain CVV
    private final String network;          // VISA, MASTERCARD, etc.

    public CreditCard(String methodId, String userId, String cardNumber,
                      String cardHolderName, YearMonth expiryDate,
                      String cvvHash, String network) {
        super(methodId, userId, PaymentMethodType.CREDIT_CARD);
        this.cardNumber = cardNumber;
        this.cardHolderName = cardHolderName;
        this.expiryDate = expiryDate;
        this.cvvHash = cvvHash;
        this.network = network;
    }

    public String getCardNumber() { return cardNumber; }
    public String getCardHolderName() { return cardHolderName; }
    public YearMonth getExpiryDate() { return expiryDate; }
    public String getNetwork() { return network; }

    @Override
    public boolean isValid() {
        return expiryDate.isAfter(YearMonth.now()) || expiryDate.equals(YearMonth.now());
    }

    @Override
    public String toString() {
        return "CreditCard{last4='" + cardNumber + "', network='" + network + "', valid=" + isValid() + "}";
    }
}
