#include "expirybud/Models.h"

#include <stdexcept>

namespace expirybud {

std::string toString(ExpiryStatus status) {
    switch (status) {
        case ExpiryStatus::Safe:
            return "Safe";
        case ExpiryStatus::Expiring:
            return "Expiring";
        case ExpiryStatus::Expired:
            return "Expired";
    }
    throw std::invalid_argument("Unknown expiry status");
}

std::string toString(InventoryOutcome outcome) {
    switch (outcome) {
        case InventoryOutcome::Active:
            return "Active";
        case InventoryOutcome::Consumed:
            return "Consumed";
        case InventoryOutcome::Discarded:
            return "Discarded";
        case InventoryOutcome::ExpiredUnused:
            return "ExpiredUnused";
    }
    throw std::invalid_argument("Unknown inventory outcome");
}

InventoryOutcome inventoryOutcomeFromString(const std::string& value) {
    if (value == "Active") {
        return InventoryOutcome::Active;
    }
    if (value == "Consumed") {
        return InventoryOutcome::Consumed;
    }
    if (value == "Discarded") {
        return InventoryOutcome::Discarded;
    }
    if (value == "ExpiredUnused") {
        return InventoryOutcome::ExpiredUnused;
    }
    throw std::invalid_argument("Unknown inventory outcome: " + value);
}

ExpiryStatus Product::expiryStatus(const Date& today, int alertThresholdDays) const {
    const int remaining = daysRemaining(today);
    if (remaining < 0) {
        return ExpiryStatus::Expired;
    }
    if (remaining <= alertThresholdDays) {
        return ExpiryStatus::Expiring;
    }
    return ExpiryStatus::Safe;
}

int Product::daysRemaining(const Date& today) const {
    return today.daysUntil(expiryDate);
}

double Product::estimatedValue() const {
    return unitPrice.has_value() ? quantity * *unitPrice : 0.0;
}

}  // namespace expirybud
