#include "expirybud/Application.h"
#include "expirybud/Text.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace expirybud {

namespace {

double parseCompleteDouble(const std::string& value) {
    std::size_t parsed = 0;
    const double result = std::stod(value, &parsed);
    if (parsed != value.size() || !std::isfinite(result)) {
        throw std::invalid_argument("Invalid number");
    }
    return result;
}

std::string shorten(const std::string& value, std::size_t maximum) {
    if (value.size() <= maximum) {
        return value;
    }
    return value.substr(0, maximum - 3) + "...";
}

}  // namespace

Application::Application(IDataStore& dataStore,
                         IAlertService& alertService,
                         std::istream& input,
                         std::ostream& output)
    : dataStore_(dataStore),
      alertService_(alertService),
      input_(input),
      output_(output) {}

int Application::run() {
    load();
    const Date today = Date::today();
    const std::size_t archived = ExpiryService::archiveExpired(state_, today);
    if (archived > 0) {
        persistAll();
        output_ << archived << " expired product" << (archived == 1 ? " was" : "s were")
                << " moved to history.\n";
    }

    output_ << "\nExpiry Bud - Household Product Expiry Monitor\n";
    output_ << "Today: " << today.toString() << '\n';
    alertService_.show(state_.activeProducts, today, state_.settings.alertThresholdDays);

    while (true) {
        try {
            showMenu();
            const int choice = promptInt("Choose an option: ", 0, 13);
            switch (choice) {
                case 0:
                    persistAll();
                    output_ << "Data saved. Goodbye.\n";
                    return 0;
                case 1:
                    showDashboard();
                    break;
                case 2:
                    addProduct();
                    break;
                case 3:
                    listActive();
                    break;
                case 4:
                    searchProducts();
                    break;
                case 5:
                    sortAndFilter();
                    break;
                case 6:
                    updateProduct();
                    break;
                case 7:
                    markOutcome(InventoryOutcome::Consumed);
                    break;
                case 8:
                    markOutcome(InventoryOutcome::Discarded);
                    break;
                case 9:
                    deleteProduct();
                    break;
                case 10:
                    showHistory();
                    break;
                case 11:
                    showAnalytics();
                    break;
                case 12:
                    manageCategories();
                    break;
                case 13:
                    changeSettings();
                    break;
            }
        } catch (const std::exception& error) {
            if (std::string(error.what()) == "Input stream closed") {
                persistAll();
                output_ << "Input closed. Data saved.\n";
                return 0;
            }
            output_ << "Error: " << error.what() << "\n";
        }
    }
}

void Application::load() {
    state_.activeProducts = dataStore_.loadProducts();
    state_.history = dataStore_.loadHistory();
    state_.categories = dataStore_.loadCategories();
    state_.settings = dataStore_.loadSettings();
    if (state_.categories.empty()) {
        state_.categories = {"Food", "Medicine", "Cosmetics", "Dairy", "Grocery", "Household"};
    }
    persistAll();
}

void Application::persistAll() {
    dataStore_.saveProducts(state_.activeProducts);
    dataStore_.saveHistory(state_.history);
    dataStore_.saveCategories(state_.categories);
    dataStore_.saveSettings(state_.settings);
}

void Application::showMenu() const {
    output_ << "\nMain menu\n"
            << " 1. Dashboard and alerts\n"
            << " 2. Add product\n"
            << " 3. View active products\n"
            << " 4. Search products\n"
            << " 5. Sort or filter products\n"
            << " 6. Update product\n"
            << " 7. Mark product consumed\n"
            << " 8. Mark product discarded\n"
            << " 9. Delete product\n"
            << "10. View history\n"
            << "11. View waste analytics\n"
            << "12. Manage categories\n"
            << "13. Change alert threshold\n"
            << " 0. Save and exit\n";
}

void Application::showDashboard() {
    const Date today = Date::today();
    int safe = 0;
    int expiring = 0;
    int expired = 0;
    for (const Product& product : state_.activeProducts) {
        switch (product.expiryStatus(today, state_.settings.alertThresholdDays)) {
            case ExpiryStatus::Safe:
                ++safe;
                break;
            case ExpiryStatus::Expiring:
                ++expiring;
                break;
            case ExpiryStatus::Expired:
                ++expired;
                break;
        }
    }
    output_ << "\nDashboard\n"
            << "Active: " << state_.activeProducts.size() << " | Safe: " << safe
            << " | Expiring: " << expiring << " | Expired pending archive: " << expired
            << " | History: " << state_.history.size() << '\n';
    alertService_.show(state_.activeProducts, today, state_.settings.alertThresholdDays);
}

void Application::addProduct() {
    Product product;
    product.name = promptRequired("Product name: ");
    product.category = chooseCategory();
    product.manufactureDate = promptOptionalDate("Manufacture date DD-MM-YYYY (optional): ");
    product.expiryDate = promptDate("Expiry date DD-MM-YYYY: ");
    product.quantity = promptPositiveDouble("Quantity: ");
    product.unit = promptRequired("Unit (item, kg, L, etc.): ");
    product.unitPrice = promptOptionalNonNegativeDouble("Unit price in Rs. (optional): ");
    product.dateAdded = Date::today();
    product.outcome = InventoryOutcome::Active;
    ProductService::add(state_, product);
    dataStore_.saveProducts(state_.activeProducts);
    output_ << "Product added with ID " << state_.activeProducts.back().id << ".\n";
}

void Application::listActive() const {
    printProducts(SearchService::sortByExpiry(state_.activeProducts), false);
}

void Application::searchProducts() const {
    const std::string query = promptRequired("Search by product name or category: ");
    printProducts(SearchService::sortByExpiry(
                      SearchService::search(state_.activeProducts, query)),
                  false);
}

void Application::sortAndFilter() const {
    output_ << "1. All by nearest expiry\n"
            << "2. Safe\n"
            << "3. Expiring\n"
            << "4. Expired\n";
    const int choice = promptInt("Choose a filter: ", 1, 4);
    std::vector<Product> products = state_.activeProducts;
    if (choice > 1) {
        const ExpiryStatus status = choice == 2   ? ExpiryStatus::Safe
                                    : choice == 3 ? ExpiryStatus::Expiring
                                                  : ExpiryStatus::Expired;
        products = SearchService::filterByStatus(
            products, status, Date::today(), state_.settings.alertThresholdDays);
    }
    printProducts(SearchService::sortByExpiry(std::move(products)), false);
}

void Application::updateProduct() {
    const std::string id = promptRequired("Product ID to update: ");
    Product* existing = ProductService::findActive(state_, id);
    if (existing == nullptr) {
        output_ << "Product not found.\n";
        return;
    }

    Product updated = *existing;
    std::string value = prompt("Name [" + existing->name + "] (blank keeps): ");
    if (!trim(value).empty()) {
        updated.name = trim(value);
    }

    output_ << "Categories:\n";
    for (std::size_t index = 0; index < state_.categories.size(); ++index) {
        output_ << index + 1 << ". " << state_.categories[index] << '\n';
    }
    const int categoryChoice = promptInt("Category number (0 keeps current): ", 0,
                                         static_cast<int>(state_.categories.size()));
    if (categoryChoice > 0) {
        updated.category = state_.categories[static_cast<std::size_t>(categoryChoice - 1)];
    }

    value = trim(prompt("Manufacture date (blank keeps, '-' clears): "));
    if (value == "-") {
        updated.manufactureDate.reset();
    } else if (!value.empty()) {
        updated.manufactureDate = Date::parse(value);
    }

    value = trim(prompt("Expiry date (blank keeps): "));
    if (!value.empty()) {
        updated.expiryDate = Date::parse(value);
    }

    value = trim(prompt("Quantity (blank keeps): "));
    if (!value.empty()) {
        updated.quantity = parseCompleteDouble(value);
    }

    value = trim(prompt("Unit (blank keeps): "));
    if (!value.empty()) {
        updated.unit = value;
    }

    value = trim(prompt("Unit price (blank keeps, '-' clears): "));
    if (value == "-") {
        updated.unitPrice.reset();
    } else if (!value.empty()) {
        updated.unitPrice = parseCompleteDouble(value);
    }

    ProductService::validate(updated, state_.categories);
    *existing = updated;
    dataStore_.saveProducts(state_.activeProducts);
    output_ << "Product updated.\n";
}

void Application::markOutcome(InventoryOutcome outcome) {
    const std::string id = promptRequired("Product ID: ");
    if (!HistoryService::archiveWithOutcome(state_, id, outcome, Date::today())) {
        output_ << "Product not found.\n";
        return;
    }
    dataStore_.saveProducts(state_.activeProducts);
    dataStore_.saveHistory(state_.history);
    output_ << "Product moved to history as " << toString(outcome) << ".\n";
}

void Application::deleteProduct() {
    const std::string id = promptRequired("Product ID to delete permanently: ");
    const std::string confirmation = lowercase(prompt("Type DELETE to confirm: "));
    if (confirmation != "delete") {
        output_ << "Deletion cancelled.\n";
        return;
    }
    if (!ProductService::remove(state_, id)) {
        output_ << "Product not found.\n";
        return;
    }
    dataStore_.saveProducts(state_.activeProducts);
    output_ << "Product deleted.\n";
}

void Application::showHistory() const {
    std::vector<Product> history = state_.history;
    std::stable_sort(history.begin(), history.end(), [](const Product& left, const Product& right) {
        if (!left.outcomeDate) {
            return false;
        }
        if (!right.outcomeDate) {
            return true;
        }
        return *right.outcomeDate < *left.outcomeDate;
    });
    printProducts(history, true);
}

void Application::showAnalytics() const {
    const AnalyticsSummary summary = AnalyticsService::summarize(state_.history);
    output_ << std::fixed << std::setprecision(2)
            << "\nWaste analytics\n"
            << "Completed items: " << summary.completedItems << '\n'
            << "Wasted items: " << summary.wastedItems << '\n'
            << "Waste rate: " << summary.wasteRatePercent << "%\n"
            << "Estimated loss: Rs. " << summary.estimatedLoss << '\n';
    if (summary.mostWastedCategory) {
        output_ << "Most-wasted category: " << *summary.mostWastedCategory << '\n';
    }

    output_ << "\nBy category\n";
    for (const auto& entry : summary.byCategory) {
        output_ << "- " << entry.first << ": " << entry.second.wastedItems
                << " item(s), Rs. " << entry.second.estimatedLoss << '\n';
    }
    output_ << "\nBy month\n";
    for (const auto& entry : summary.byMonth) {
        output_ << "- " << entry.first << ": " << entry.second.wastedItems
                << " item(s), Rs. " << entry.second.estimatedLoss << '\n';
    }
    output_ << std::defaultfloat;
}

void Application::manageCategories() {
    output_ << "\nCategories\n";
    for (std::size_t index = 0; index < state_.categories.size(); ++index) {
        output_ << index + 1 << ". " << state_.categories[index] << '\n';
    }
    output_ << "1. Add category\n2. Remove unused category\n0. Return\n";
    const int choice = promptInt("Choose: ", 0, 2);
    if (choice == 0) {
        return;
    }
    if (choice == 1) {
        const std::string name = promptRequired("New category name: ");
        const bool duplicate = std::any_of(
            state_.categories.begin(), state_.categories.end(),
            [&](const std::string& category) { return equalsIgnoreCase(category, name); });
        if (duplicate) {
            throw std::invalid_argument("Category already exists");
        }
        state_.categories.push_back(trim(name));
    } else {
        if (state_.categories.size() == 1) {
            throw std::invalid_argument("At least one category must remain available");
        }
        const int number = promptInt("Category number to remove: ", 1,
                                     static_cast<int>(state_.categories.size()));
        const std::string& category = state_.categories[static_cast<std::size_t>(number - 1)];
        const auto used = [&](const std::vector<Product>& products) {
            return std::any_of(products.begin(), products.end(), [&](const Product& product) {
                return equalsIgnoreCase(product.category, category);
            });
        };
        if (used(state_.activeProducts) || used(state_.history)) {
            throw std::invalid_argument("Cannot remove a category used by product records");
        }
        state_.categories.erase(state_.categories.begin() + number - 1);
    }
    dataStore_.saveCategories(state_.categories);
    output_ << "Categories updated.\n";
}

void Application::changeSettings() {
    state_.settings.alertThresholdDays =
        promptInt("Alert threshold in days (0-365): ", 0, 365);
    dataStore_.saveSettings(state_.settings);
    output_ << "Alert threshold updated.\n";
}

std::string Application::prompt(const std::string& message) const {
    output_ << message;
    std::string value;
    if (!std::getline(input_, value)) {
        throw std::runtime_error("Input stream closed");
    }
    return value;
}

std::string Application::promptRequired(const std::string& message) const {
    while (true) {
        const std::string value = trim(prompt(message));
        if (!value.empty()) {
            return value;
        }
        output_ << "A value is required.\n";
    }
}

int Application::promptInt(const std::string& message, int minimum, int maximum) const {
    while (true) {
        const std::string value = trim(prompt(message));
        try {
            std::size_t parsed = 0;
            const int result = std::stoi(value, &parsed);
            if (parsed == value.size() && result >= minimum && result <= maximum) {
                return result;
            }
        } catch (const std::exception&) {
        }
        output_ << "Enter a whole number from " << minimum << " to " << maximum << ".\n";
    }
}

double Application::promptPositiveDouble(const std::string& message) const {
    while (true) {
        try {
            const double value = parseCompleteDouble(trim(prompt(message)));
            if (value > 0.0) {
                return value;
            }
        } catch (const std::exception&) {
        }
        output_ << "Enter a number greater than zero.\n";
    }
}

std::optional<double> Application::promptOptionalNonNegativeDouble(
    const std::string& message) const {
    while (true) {
        const std::string input = trim(prompt(message));
        if (input.empty()) {
            return std::nullopt;
        }
        try {
            const double value = parseCompleteDouble(input);
            if (value >= 0.0) {
                return value;
            }
        } catch (const std::exception&) {
        }
        output_ << "Enter zero or a positive number, or leave it blank.\n";
    }
}

Date Application::promptDate(const std::string& message) const {
    while (true) {
        try {
            return Date::parse(trim(prompt(message)));
        } catch (const std::exception& error) {
            output_ << error.what() << ".\n";
        }
    }
}

std::optional<Date> Application::promptOptionalDate(const std::string& message) const {
    while (true) {
        const std::string value = trim(prompt(message));
        if (value.empty()) {
            return std::nullopt;
        }
        try {
            return Date::parse(value);
        } catch (const std::exception& error) {
            output_ << error.what() << ".\n";
        }
    }
}

std::string Application::chooseCategory() const {
    output_ << "Categories:\n";
    for (std::size_t index = 0; index < state_.categories.size(); ++index) {
        output_ << index + 1 << ". " << state_.categories[index] << '\n';
    }
    const int choice = promptInt("Category number: ", 1,
                                 static_cast<int>(state_.categories.size()));
    return state_.categories[static_cast<std::size_t>(choice - 1)];
}

void Application::printProducts(const std::vector<Product>& products, bool includeOutcome) const {
    if (products.empty()) {
        output_ << "No products found.\n";
        return;
    }

    const Date today = Date::today();
    output_ << '\n' << std::left << std::setw(9) << "ID" << std::setw(23) << "Name"
            << std::setw(15) << "Category" << std::setw(13) << "Expiry"
            << std::setw(11) << (includeOutcome ? "Outcome" : "Status") << std::setw(10)
            << "Quantity" << std::setw(10) << "Price" << '\n';
    output_ << std::string(91, '-') << '\n';

    for (const Product& product : products) {
        std::ostringstream quantity;
        quantity << product.quantity << ' ' << product.unit;
        std::ostringstream price;
        if (product.unitPrice) {
            price << std::fixed << std::setprecision(2) << *product.unitPrice;
        } else {
            price << '-';
        }
        output_ << std::left << std::setw(9) << product.id << std::setw(23)
                << shorten(product.name, 21) << std::setw(15) << shorten(product.category, 13)
                << std::setw(13) << product.expiryDate.toString() << std::setw(11)
                << (includeOutcome
                        ? shorten(toString(product.outcome), 9)
                        : toString(product.expiryStatus(today, state_.settings.alertThresholdDays)))
                << std::setw(10) << shorten(quantity.str(), 8) << std::setw(10) << price.str()
                << '\n';
    }
    output_ << std::right;
}

}  // namespace expirybud
