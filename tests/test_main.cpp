#include "expirybud/Application.h"
#include "expirybud/Date.h"
#include "expirybud/Models.h"
#include "expirybud/Services.h"
#include "expirybud/Storage.h"
#include "expirybud/Text.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace expirybud;

namespace {

class TestSuite {
public:
    void run(const std::string& name, const std::function<void()>& test) {
        try {
            test();
            ++passed_;
            std::cout << "[PASS] " << name << '\n';
        } catch (const std::exception& error) {
            ++failed_;
            std::cout << "[FAIL] " << name << ": " << error.what() << '\n';
        }
    }

    int result() const {
        std::cout << "\nTests: " << passed_ + failed_ << ", passed: " << passed_
                  << ", failed: " << failed_ << '\n';
        return failed_ == 0 ? 0 : 1;
    }

private:
    int passed_{0};
    int failed_{0};
};

void check(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename Callable>
void checkThrows(Callable callable, const std::string& message) {
    try {
        callable();
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error(message);
}

void checkNear(double actual, double expected, const std::string& message) {
    if (std::fabs(actual - expected) > 0.0001) {
        throw std::runtime_error(message);
    }
}

Product makeProduct(std::string id,
                    std::string name,
                    std::string category,
                    Date expiry,
                    double quantity = 1.0,
                    std::optional<double> unitPrice = std::nullopt) {
    Product product;
    product.id = std::move(id);
    product.name = std::move(name);
    product.category = std::move(category);
    product.expiryDate = expiry;
    product.quantity = quantity;
    product.unit = "item";
    product.unitPrice = unitPrice;
    product.dateAdded = Date(2026, 1, 1);
    return product;
}

std::filesystem::path uniqueTempDirectory() {
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path() /
                      ("expirybud_tests_" + std::to_string(stamp));
    std::filesystem::create_directories(path);
    return path;
}

}  // namespace

int main() {
    TestSuite suite;

    suite.run("Date parsing, formatting, and leap years", [] {
        const Date leap = Date::parse("29-02-2024");
        check(leap.toString() == "29-02-2024", "Leap date did not round-trip");
        checkThrows([] { Date::parse("29-02-2023"); }, "Invalid leap date was accepted");
        checkThrows([] { Date::parse("2026/01/01"); }, "Invalid date format was accepted");
    });

    suite.run("Date difference across month and year boundaries", [] {
        check(Date(2026, 12, 31).daysUntil(Date(2027, 1, 1)) == 1,
              "Year boundary difference is incorrect");
        check(Date(2024, 2, 28).daysUntil(Date(2024, 3, 1)) == 2,
              "Leap-year difference is incorrect");
    });

    suite.run("Expiry status boundary rules", [] {
        const Date today(2026, 10, 31);
        Product todayProduct = makeProduct("P1", "Milk", "Dairy", today);
        Product threshold = makeProduct("P2", "Bread", "Food", Date(2026, 11, 7));
        Product safe = makeProduct("P3", "Rice", "Grocery", Date(2026, 11, 8));
        Product expired = makeProduct("P4", "Cream", "Cosmetics", Date(2026, 10, 30));
        check(todayProduct.expiryStatus(today, 7) == ExpiryStatus::Expiring,
              "Expiry today should be Expiring");
        check(threshold.expiryStatus(today, 7) == ExpiryStatus::Expiring,
              "Threshold boundary should be Expiring");
        check(safe.expiryStatus(today, 7) == ExpiryStatus::Safe,
              "Beyond threshold should be Safe");
        check(expired.expiryStatus(today, 7) == ExpiryStatus::Expired,
              "Past date should be Expired");
    });

    suite.run("Product validation accepts valid data and rejects invalid data", [] {
        const std::vector<std::string> categories{"Food", "Medicine"};
        Product valid = makeProduct("P1", "Bread", "Food", Date(2026, 12, 1));
        valid.manufactureDate = Date(2026, 11, 1);
        ProductService::validate(valid, categories);

        Product invalidQuantity = valid;
        invalidQuantity.quantity = 0;
        checkThrows([&] { ProductService::validate(invalidQuantity, categories); },
                    "Zero quantity was accepted");
        Product invalidCategory = valid;
        invalidCategory.category = "Unknown";
        checkThrows([&] { ProductService::validate(invalidCategory, categories); },
                    "Unknown category was accepted");
        Product invalidDates = valid;
        invalidDates.manufactureDate = invalidDates.expiryDate;
        checkThrows([&] { ProductService::validate(invalidDates, categories); },
                    "Equal manufacture and expiry dates were accepted");
    });

    suite.run("Product IDs remain unique across active products and history", [] {
        AppState state;
        state.categories = {"Food"};
        state.activeProducts.push_back(makeProduct("P000009", "Bread", "Food", Date(2027, 1, 1)));
        Product history = makeProduct("P000012", "Rice", "Food", Date(2026, 1, 1));
        history.outcome = InventoryOutcome::Consumed;
        state.history.push_back(history);
        check(ProductService::nextId(state) == "P000013", "Next ID ignored history");
        Product duplicate = makeProduct("P000012", "Duplicate", "Food", Date(2027, 2, 1));
        checkThrows([&] { ProductService::add(state, duplicate); },
                    "ID duplicated a history record");
    });

    suite.run("Search is case-insensitive and sorting uses nearest expiry", [] {
        std::vector<Product> products{
            makeProduct("P1", "Face Cream", "Cosmetics", Date(2027, 1, 1)),
            makeProduct("P2", "Almond Milk", "Dairy", Date(2026, 11, 1)),
            makeProduct("P3", "Paneer", "Dairy", Date(2026, 11, 3))};
        const auto found = SearchService::search(products, "DAIRY");
        check(found.size() == 2, "Category search returned the wrong count");
        const auto sorted = SearchService::sortByExpiry(products);
        check(sorted.front().id == "P2" && sorted.back().id == "P1",
              "Products were not sorted by expiry");
    });

    suite.run("Expired products archive once", [] {
        AppState state;
        state.settings.alertThresholdDays = 7;
        state.activeProducts = {
            makeProduct("P1", "Old Milk", "Dairy", Date(2026, 10, 30)),
            makeProduct("P2", "Fresh Milk", "Dairy", Date(2026, 11, 10))};
        const Date today(2026, 10, 31);
        check(ExpiryService::archiveExpired(state, today) == 1, "Expired product was not archived");
        check(state.activeProducts.size() == 1 && state.history.size() == 1,
              "Archive changed the wrong records");
        check(state.history.front().outcome == InventoryOutcome::ExpiredUnused,
              "Archived product has the wrong outcome");
        check(ExpiryService::archiveExpired(state, today) == 0,
              "Product was archived more than once");
    });

    suite.run("Consumed and discarded outcomes move products to history", [] {
        AppState state;
        state.activeProducts = {
            makeProduct("P1", "Bread", "Food", Date(2026, 12, 1)),
            makeProduct("P2", "Milk", "Dairy", Date(2026, 12, 2))};
        check(HistoryService::archiveWithOutcome(
                  state, "P1", InventoryOutcome::Consumed, Date(2026, 11, 1)),
              "Consumed product was not archived");
        check(HistoryService::archiveWithOutcome(
                  state, "P2", InventoryOutcome::Discarded, Date(2026, 11, 2)),
              "Discarded product was not archived");
        check(state.activeProducts.empty() && state.history.size() == 2,
              "History transition left incorrect state");
    });

    suite.run("Analytics calculates counts, loss, rate, category, and month", [] {
        Product consumed = makeProduct("P1", "Bread", "Food", Date(2026, 12, 1), 1, 30.0);
        consumed.outcome = InventoryOutcome::Consumed;
        consumed.outcomeDate = Date(2026, 11, 1);
        Product discarded = makeProduct("P2", "Milk", "Dairy", Date(2026, 11, 2), 2, 40.0);
        discarded.outcome = InventoryOutcome::Discarded;
        discarded.outcomeDate = Date(2026, 11, 2);
        Product expired = makeProduct("P3", "Yogurt", "Dairy", Date(2026, 10, 30), 1, 20.0);
        expired.outcome = InventoryOutcome::ExpiredUnused;
        expired.outcomeDate = Date(2026, 11, 3);

        const AnalyticsSummary summary =
            AnalyticsService::summarize({consumed, discarded, expired});
        check(summary.completedItems == 3, "Completed count is incorrect");
        check(summary.wastedItems == 2, "Waste count is incorrect");
        checkNear(summary.wasteRatePercent, 66.666666, "Waste rate is incorrect");
        checkNear(summary.estimatedLoss, 100.0, "Estimated loss is incorrect");
        check(summary.mostWastedCategory == std::optional<std::string>("Dairy"),
              "Most-wasted category is incorrect");
        check(summary.byMonth.at("11-2026").wastedItems == 2,
              "Monthly waste count is incorrect");
    });

    suite.run("CSV storage round-trips quoted values and optional fields", [] {
        const auto directory = uniqueTempDirectory();
        try {
            CsvDataStore store(directory);
            Product product = makeProduct("P000001", "Milk, \"Premium\"", "Dairy",
                                          Date(2026, 12, 1), 2.5, 42.75);
            product.manufactureDate = Date(2026, 11, 1);
            store.saveProducts({product});
            const auto loaded = store.loadProducts();
            check(loaded.size() == 1, "Storage returned the wrong product count");
            check(loaded.front().name == product.name, "Quoted name did not round-trip");
            check(loaded.front().manufactureDate == product.manufactureDate,
                  "Optional manufacture date did not round-trip");
            checkNear(*loaded.front().unitPrice, 42.75, "Unit price did not round-trip");
        } catch (...) {
            std::filesystem::remove_all(directory);
            throw;
        }
        std::filesystem::remove_all(directory);
    });

    suite.run("Settings and categories persist", [] {
        const auto directory = uniqueTempDirectory();
        try {
            CsvDataStore store(directory);
            store.saveCategories({"Food", "Pet Care"});
            store.saveSettings(AppSettings{14});
            check(store.loadCategories() == std::vector<std::string>({"Food", "Pet Care"}),
                  "Categories did not persist");
            check(store.loadSettings().alertThresholdDays == 14,
                  "Alert threshold did not persist");
        } catch (...) {
            std::filesystem::remove_all(directory);
            throw;
        }
        std::filesystem::remove_all(directory);
    });

    suite.run("Corrupted CSV records report their line number", [] {
        const auto directory = uniqueTempDirectory();
        try {
            std::ofstream output(directory / "products.csv");
            output << "header\ninvalid,row\n";
            output.close();
            CsvDataStore store(directory);
            checkThrows([&] { store.loadProducts(); }, "Corrupted CSV was accepted");
        } catch (...) {
            std::filesystem::remove_all(directory);
            throw;
        }
        std::filesystem::remove_all(directory);
    });

    suite.run("Console alerts include products expiring today", [] {
        std::ostringstream output;
        ConsoleAlertService alerts(output);
        alerts.show({makeProduct("P1", "Milk", "Dairy", Date(2026, 10, 31))},
                    Date(2026, 10, 31), 7);
        check(output.str().find("expires today") != std::string::npos,
              "Today's expiry alert was not displayed");
    });

    suite.run("Application starts, creates data files, and exits cleanly", [] {
        const auto directory = uniqueTempDirectory();
        try {
            CsvDataStore store(directory);
            std::istringstream input("0\n");
            std::ostringstream output;
            ConsoleAlertService alerts(output);
            Application application(store, alerts, input, output);
            check(application.run() == 0, "Application returned a failure status");
            check(std::filesystem::exists(directory / "products.csv"),
                  "Application did not create products.csv");
            check(std::filesystem::exists(directory / "history.csv"),
                  "Application did not create history.csv");
            check(output.str().find("Data saved. Goodbye.") != std::string::npos,
                  "Application did not exit cleanly");
        } catch (...) {
            std::filesystem::remove_all(directory);
            throw;
        }
        std::filesystem::remove_all(directory);
    });

    suite.run("Application prevents removal of the final category", [] {
        const auto directory = uniqueTempDirectory();
        try {
            CsvDataStore store(directory);
            store.saveCategories({"Food"});
            std::istringstream input("12\n2\n0\n");
            std::ostringstream output;
            ConsoleAlertService alerts(output);
            Application application(store, alerts, input, output);
            check(application.run() == 0, "Application returned a failure status");
            check(store.loadCategories() == std::vector<std::string>({"Food"}),
                  "Application removed the final category");
            check(output.str().find("At least one category") != std::string::npos,
                  "Application did not explain the category constraint");
        } catch (...) {
            std::filesystem::remove_all(directory);
            throw;
        }
        std::filesystem::remove_all(directory);
    });

    suite.run("All terminal menu options work together", [] {
        const auto directory = uniqueTempDirectory();
        try {
            CsvDataStore store(directory);
            const std::string script =
                "2\nProduct A\n4\n01-01-2099\n31-12-2099\n2\nL\n50\n"
                "1\n3\n4\nProduct A\n5\n1\n"
                "6\nP000001\nProduct A Updated\n4\n\n\n3\n\n\n"
                "7\nP000001\n10\n11\n"
                "12\n1\nPet Care\n13\n14\n"
                "2\nDelete Me\n7\n\n31-12-2099\n1\nitem\n\n"
                "9\nP000002\nDELETE\n"
                "2\nDiscard Me\n1\n\n31-12-2099\n1\nitem\n10\n"
                "8\nP000002\n11\n"
                "12\n2\n7\n0\n";
            std::istringstream input(script);
            std::ostringstream output;
            ConsoleAlertService alerts(output);
            Application application(store, alerts, input, output);
            check(application.run() == 0, "Application returned a failure status");

            const auto active = store.loadProducts();
            const auto history = store.loadHistory();
            check(active.empty(), "Menu workflow left an active product");
            check(history.size() == 2, "Menu workflow created the wrong history count");
            check(history[0].name == "Product A Updated" &&
                      history[0].outcome == InventoryOutcome::Consumed,
                  "Update or consumed action failed");
            check(history[1].name == "Discard Me" &&
                      history[1].outcome == InventoryOutcome::Discarded,
                  "Discard action failed");
            check(store.loadSettings().alertThresholdDays == 14,
                  "Settings option did not persist");
            const auto categories = store.loadCategories();
            check(std::find(categories.begin(), categories.end(), "Pet Care") == categories.end(),
                  "Category add/remove workflow failed");
            const std::string transcript = output.str();
            for (const std::string& expected : {
                     "Dashboard", "Product added", "Product updated", "Product deleted",
                     "Waste analytics", "Categories updated", "Alert threshold updated"}) {
                check(transcript.find(expected) != std::string::npos,
                      "Missing terminal result: " + expected);
            }
        } catch (...) {
            std::filesystem::remove_all(directory);
            throw;
        }
        std::filesystem::remove_all(directory);
    });

    return suite.result();
}
