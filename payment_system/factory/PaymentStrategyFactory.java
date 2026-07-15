package factory;

import models.User;
import strategy.*;
import enums.PaymentMethodType;

/**
 * Factory that returns the correct PaymentStrategy based on the payment method type.
 */
public class PaymentStrategyFactory {

    public static PaymentStrategy getStrategy(PaymentMethodType type, User user) {
        return switch (type) {
            case CREDIT_CARD -> new CreditCardPaymentStrategy();
            case DEBIT_CARD  -> new DebitCardPaymentStrategy();
            case UPI         -> new UPIPaymentStrategy();
            case WALLET      -> new WalletPaymentStrategy(user);
        };
    }
}
