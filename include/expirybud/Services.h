#pragma once

#include "expirybud/Models.h"

#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace expirybud {

class ProductService {
public:
    static void validate(const Product& product, const std::vector<std::string>& categories);
    static std::string nextId(const AppState& state);
    static Product* findActive(AppState& state, const std::string& id);
    static const Product* findActive(const AppState& state, const std::string& id);
    static void add(AppState& state, Product product);
    static bool remove(AppState& state, const std::string& id);
};

class ExpiryService {
public:
    static std::size_t archiveExpired(AppState& state, const Date& today);
    static std::vector<Product> expiringProducts(const AppState& state, const Date& today);
};

class SearchService {
public:
    static std::vector<Product> search(const std::vector<Product>& products,
                                       const std::string& query);
    static std::vector<Product> filterByStatus(const std::vector<Product>& products,
                                               ExpiryStatus status,
                                               const Date& today,
                                               int thresholdDays);
    static std::vector<Product> sortByExpiry(std::vector<Product> products);
};

class HistoryService {
public:
    static bool archiveWithOutcome(AppState& state,
                                   const std::string& productId,
                                   InventoryOutcome outcome,
                                   const Date& outcomeDate);
};

struct CategorySummary {
    int wastedItems{0};
    double estimatedLoss{0.0};
};

struct AnalyticsSummary {
    int completedItems{0};
    int wastedItems{0};
    double wasteRatePercent{0.0};
    double estimatedLoss{0.0};
    std::map<std::string, CategorySummary> byCategory;
    std::map<std::string, CategorySummary> byMonth;
    std::optional<std::string> mostWastedCategory;
};

class AnalyticsService {
public:
    static AnalyticsSummary summarize(const std::vector<Product>& history);
};

class IAlertService {
public:
    virtual ~IAlertService() = default;
    virtual void show(const std::vector<Product>& products,
                      const Date& today,
                      int thresholdDays) = 0;
};

class ConsoleAlertService final : public IAlertService {
public:
    explicit ConsoleAlertService(std::ostream& output);
    void show(const std::vector<Product>& products,
              const Date& today,
              int thresholdDays) override;

private:
    std::ostream& output_;
};

}  // namespace expirybud
