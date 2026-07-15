package models;

import enums.PaymentMethodType;
import java.time.YearMonth;

public class DebitCard extends PaymentMethod {
    private final String cardNumber;
    private final String cardHolderName;
    private final YearMonth expiryDate;
    private final String bankName;

    public DebitCard(String methodId, String userId, String cardNumber,
                     String cardHolderName, YearMonth expiryDate, String bankName) {
        super(methodId, userId, PaymentMethodType.DEBIT_CARD);
        this.cardNumber = cardNumber;
        this.cardHolderName = cardHolderName;
        this.expiryDate = expiryDate;
        this.bankName = bankName;
    }

    public String getCardNumber() { return cardNumber; }
    public String getCardHolderName() { return cardHolderName; }
    public YearMonth getExpiryDate() { return expiryDate; }
    public String getBankName() { return bankName; }

    @Override
    public boolean isValid() {
        return expiryDate.isAfter(YearMonth.now()) || expiryDate.equals(YearMonth.now());
    }

    @Override
    public String toString() {
        return "DebitCard{last4='" + cardNumber + "', bank='" + bankName + "', valid=" + isValid() + "}";
    }
}
