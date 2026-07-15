package models;

import enums.PaymentMethodType;

public abstract class PaymentMethod {
    protected final String methodId;
    protected final String userId;
    protected final PaymentMethodType type;
    protected boolean isDefault;

    protected PaymentMethod(String methodId, String userId, PaymentMethodType type) {
        this.methodId = methodId;
        this.userId = userId;
        this.type = type;
        this.isDefault = false;
    }

    public String getMethodId() { return methodId; }
    public String getUserId() { return userId; }
    public PaymentMethodType getType() { return type; }
    public boolean isDefault() { return isDefault; }
    public void setDefault(boolean isDefault) { this.isDefault = isDefault; }

    public abstract boolean isValid();

    @Override
    public String toString() {
        return "PaymentMethod{methodId='" + methodId + "', type=" + type + "}";
    }
}
