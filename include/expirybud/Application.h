#pragma once

#include "expirybud/Services.h"
#include "expirybud/Storage.h"

#include <istream>
#include <ostream>
#include <string>

namespace expirybud {

class Application {
public:
    Application(IDataStore& dataStore,
                IAlertService& alertService,
                std::istream& input,
                std::ostream& output);

    int run();

private:
    IDataStore& dataStore_;
    IAlertService& alertService_;
    std::istream& input_;
    std::ostream& output_;
    AppState state_;

    void load();
    void persistAll();
    void showMenu() const;
    void showDashboard();
    void addProduct();
    void listActive() const;
    void searchProducts() const;
    void sortAndFilter() const;
    void updateProduct();
    void markOutcome(InventoryOutcome outcome);
    void deleteProduct();
    void showHistory() const;
    void showAnalytics() const;
    void manageCategories();
    void changeSettings();

    std::string prompt(const std::string& message) const;
    std::string promptRequired(const std::string& message) const;
    int promptInt(const std::string& message, int minimum, int maximum) const;
    double promptPositiveDouble(const std::string& message) const;
    std::optional<double> promptOptionalNonNegativeDouble(const std::string& message) const;
    Date promptDate(const std::string& message) const;
    std::optional<Date> promptOptionalDate(const std::string& message) const;
    std::string chooseCategory() const;

    void printProducts(const std::vector<Product>& products, bool includeOutcome) const;
};

}  // namespace expirybud
