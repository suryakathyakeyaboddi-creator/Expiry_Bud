#pragma once

#include "expirybud/Date.h"

#include <optional>
#include <string>
#include <vector>

namespace expirybud {

enum class ExpiryStatus { Safe, Expiring, Expired };
enum class InventoryOutcome { Active, Consumed, Discarded, ExpiredUnused };

std::string toString(ExpiryStatus status);
std::string toString(InventoryOutcome outcome);
InventoryOutcome inventoryOutcomeFromString(const std::string& value);

struct Product {
    std::string id;
    std::string name;
    std::string category;
    std::optional<Date> manufactureDate;
    Date expiryDate;
    double quantity{0.0};
    std::string unit;
    std::optional<double> unitPrice;
    Date dateAdded;
    InventoryOutcome outcome{InventoryOutcome::Active};
    std::optional<Date> outcomeDate;

    ExpiryStatus expiryStatus(const Date& today, int alertThresholdDays) const;
    int daysRemaining(const Date& today) const;
    double estimatedValue() const;
};

struct AppSettings {
    int alertThresholdDays{7};
};

struct AppState {
    std::vector<Product> activeProducts;
    std::vector<Product> history;
    std::vector<std::string> categories;
    AppSettings settings;
};

}  // namespace expirybud
