package models;

import java.util.ArrayList;
import java.util.List;

public class User {
    private final String userId;
    private final String name;
    private final String email;
    private final String phone;
    private Wallet wallet;
    private final List<PaymentMethod> paymentMethods;

    public User(String userId, String name, String email, String phone) {
        this.userId = userId;
        this.name = name;
        this.email = email;
        this.phone = phone;
        this.paymentMethods = new ArrayList<>();
    }

    public String getUserId() { return userId; }
    public String getName() { return name; }
    public String getEmail() { return email; }
    public String getPhone() { return phone; }

    public Wallet getWallet() { return wallet; }
    public void setWallet(Wallet wallet) { this.wallet = wallet; }

    public List<PaymentMethod> getPaymentMethods() { return paymentMethods; }

    public void addPaymentMethod(PaymentMethod method) {
        paymentMethods.add(method);
    }

    public void removePaymentMethod(String methodId) {
        paymentMethods.removeIf(m -> m.getMethodId().equals(methodId));
    }

    @Override
    public String toString() {
        return "User{userId='" + userId + "', name='" + name + "'}";
    }
}
