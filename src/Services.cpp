#include "expirybud/Services.h"
#include "expirybud/Text.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace expirybud {

namespace {

bool isWaste(InventoryOutcome outcome) {
    return outcome == InventoryOutcome::Discarded ||
           outcome == InventoryOutcome::ExpiredUnused;
}

std::string monthKey(const Date& date) {
    std::ostringstream output;
    output << std::setfill('0') << std::setw(2) << date.month() << '-' << date.year();
    return output.str();
}

}  // namespace

void ProductService::validate(const Product& product,
                              const std::vector<std::string>& categories) {
    if (trim(product.name).empty()) {
        throw std::invalid_argument("Product name is required");
    }
    if (product.name.find_first_of("\r\n") != std::string::npos) {
        throw std::invalid_argument("Product name cannot contain a line break");
    }
    const bool categoryExists = std::any_of(
        categories.begin(), categories.end(), [&](const std::string& category) {
            return equalsIgnoreCase(category, product.category);
        });
    if (!categoryExists) {
        throw std::invalid_argument("Unknown category: " + product.category);
    }
    if (product.manufactureDate && product.expiryDate <= *product.manufactureDate) {
        throw std::invalid_argument("Expiry date must be after manufacture date");
    }
    if (!std::isfinite(product.quantity) || product.quantity <= 0.0) {
        throw std::invalid_argument("Quantity must be greater than zero");
    }
    if (trim(product.unit).empty()) {
        throw std::invalid_argument("Unit is required");
    }
    if (product.unitPrice && (!std::isfinite(*product.unitPrice) || *product.unitPrice < 0.0)) {
        throw std::invalid_argument("Unit price cannot be negative");
    }
}

std::string ProductService::nextId(const AppState& state) {
    int greatest = 0;
    const auto scan = [&](const std::vector<Product>& products) {
        for (const Product& product : products) {
            if (product.id.size() < 2 || product.id.front() != 'P') {
                continue;
            }
            try {
                greatest = std::max(greatest, std::stoi(product.id.substr(1)));
            } catch (const std::exception&) {
                continue;
            }
        }
    };
    scan(state.activeProducts);
    scan(state.history);

    std::ostringstream output;
    output << 'P' << std::setfill('0') << std::setw(6) << greatest + 1;
    return output.str();
}

Product* ProductService::findActive(AppState& state, const std::string& id) {
    const auto match = std::find_if(
        state.activeProducts.begin(), state.activeProducts.end(),
        [&](const Product& product) { return equalsIgnoreCase(product.id, id); });
    return match == state.activeProducts.end() ? nullptr : &*match;
}

const Product* ProductService::findActive(const AppState& state, const std::string& id) {
    const auto match = std::find_if(
        state.activeProducts.begin(), state.activeProducts.end(),
        [&](const Product& product) { return equalsIgnoreCase(product.id, id); });
    return match == state.activeProducts.end() ? nullptr : &*match;
}

void ProductService::add(AppState& state, Product product) {
    if (product.id.empty()) {
        product.id = nextId(state);
    }
    const bool existsInHistory = std::any_of(
        state.history.begin(), state.history.end(), [&](const Product& existing) {
            return equalsIgnoreCase(existing.id, product.id);
        });
    if (ProductService::findActive(state, product.id) != nullptr || existsInHistory) {
        throw std::invalid_argument("Duplicate product ID: " + product.id);
    }
    validate(product, state.categories);
    state.activeProducts.push_back(std::move(product));
}

bool ProductService::remove(AppState& state, const std::string& id) {
    const auto originalSize = state.activeProducts.size();
    state.activeProducts.erase(
        std::remove_if(state.activeProducts.begin(), state.activeProducts.end(),
                       [&](const Product& product) { return equalsIgnoreCase(product.id, id); }),
        state.activeProducts.end());
    return state.activeProducts.size() != originalSize;
}

std::size_t ExpiryService::archiveExpired(AppState& state, const Date& today) {
    std::size_t archived = 0;
    auto current = state.activeProducts.begin();
    while (current != state.activeProducts.end()) {
        if (current->expiryStatus(today, state.settings.alertThresholdDays) !=
            ExpiryStatus::Expired) {
            ++current;
            continue;
        }

        const bool alreadyArchived = std::any_of(
            state.history.begin(), state.history.end(), [&](const Product& product) {
                return equalsIgnoreCase(product.id, current->id);
            });
        if (!alreadyArchived) {
            Product archivedProduct = *current;
            archivedProduct.outcome = InventoryOutcome::ExpiredUnused;
            archivedProduct.outcomeDate = today;
            state.history.push_back(std::move(archivedProduct));
            ++archived;
        }
        current = state.activeProducts.erase(current);
    }
    return archived;
}

std::vector<Product> ExpiryService::expiringProducts(const AppState& state,
                                                     const Date& today) {
    return SearchService::sortByExpiry(SearchService::filterByStatus(
        state.activeProducts, ExpiryStatus::Expiring, today,
        state.settings.alertThresholdDays));
}

std::vector<Product> SearchService::search(const std::vector<Product>& products,
                                           const std::string& query) {
    std::vector<Product> results;
    std::copy_if(products.begin(), products.end(), std::back_inserter(results),
                 [&](const Product& product) {
                     return containsIgnoreCase(product.name, query) ||
                            containsIgnoreCase(product.category, query);
                 });
    return results;
}

std::vector<Product> SearchService::filterByStatus(const std::vector<Product>& products,
                                                   ExpiryStatus status,
                                                   const Date& today,
                                                   int thresholdDays) {
    std::vector<Product> results;
    std::copy_if(products.begin(), products.end(), std::back_inserter(results),
                 [&](const Product& product) {
                     return product.expiryStatus(today, thresholdDays) == status;
                 });
    return results;
}

std::vector<Product> SearchService::sortByExpiry(std::vector<Product> products) {
    std::stable_sort(products.begin(), products.end(),
                     [](const Product& left, const Product& right) {
                         return left.expiryDate < right.expiryDate;
                     });
    return products;
}

bool HistoryService::archiveWithOutcome(AppState& state,
                                        const std::string& productId,
                                        InventoryOutcome outcome,
                                        const Date& outcomeDate) {
    if (outcome == InventoryOutcome::Active) {
        throw std::invalid_argument("History outcome cannot be Active");
    }
    const auto current = std::find_if(
        state.activeProducts.begin(), state.activeProducts.end(),
        [&](const Product& product) { return equalsIgnoreCase(product.id, productId); });
    if (current == state.activeProducts.end()) {
        return false;
    }
    const bool duplicate = std::any_of(
        state.history.begin(), state.history.end(), [&](const Product& product) {
            return equalsIgnoreCase(product.id, productId);
        });
    if (duplicate) {
        throw std::logic_error("Product is already present in history");
    }

    Product archived = *current;
    archived.outcome = outcome;
    archived.outcomeDate = outcomeDate;
    state.history.push_back(std::move(archived));
    state.activeProducts.erase(current);
    return true;
}

AnalyticsSummary AnalyticsService::summarize(const std::vector<Product>& history) {
    AnalyticsSummary summary;
    summary.completedItems = static_cast<int>(history.size());

    for (const Product& product : history) {
        if (!isWaste(product.outcome)) {
            continue;
        }
        ++summary.wastedItems;
        summary.estimatedLoss += product.estimatedValue();

        CategorySummary& category = summary.byCategory[product.category];
        ++category.wastedItems;
        category.estimatedLoss += product.estimatedValue();

        if (product.outcomeDate) {
            CategorySummary& month = summary.byMonth[monthKey(*product.outcomeDate)];
            ++month.wastedItems;
            month.estimatedLoss += product.estimatedValue();
        }
    }

    if (summary.completedItems > 0) {
        summary.wasteRatePercent =
            100.0 * summary.wastedItems / static_cast<double>(summary.completedItems);
    }

    int greatestWasteCount = 0;
    for (const auto& entry : summary.byCategory) {
        if (entry.second.wastedItems > greatestWasteCount) {
            greatestWasteCount = entry.second.wastedItems;
            summary.mostWastedCategory = entry.first;
        }
    }
    return summary;
}

ConsoleAlertService::ConsoleAlertService(std::ostream& output) : output_(output) {}

void ConsoleAlertService::show(const std::vector<Product>& products,
                               const Date& today,
                               int thresholdDays) {
    const auto expiring = SearchService::sortByExpiry(
        SearchService::filterByStatus(products, ExpiryStatus::Expiring, today, thresholdDays));
    if (expiring.empty()) {
        output_ << "No products are nearing expiry.\n";
        return;
    }

    output_ << "\nExpiry alerts:\n";
    for (const Product& product : expiring) {
        const int remaining = product.daysRemaining(today);
        output_ << "- " << product.name << " (" << product.id << "): ";
        if (remaining == 0) {
            output_ << "expires today";
        } else {
            output_ << remaining << " day" << (remaining == 1 ? "" : "s") << " remaining";
        }
        output_ << '\n';
    }
}

}  // namespace expirybud
