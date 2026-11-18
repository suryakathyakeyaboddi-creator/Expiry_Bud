#pragma once

#include "expirybud/Models.h"

#include <filesystem>
#include <vector>

namespace expirybud {

class IDataStore {
public:
    virtual ~IDataStore() = default;

    virtual std::vector<Product> loadProducts() = 0;
    virtual std::vector<Product> loadHistory() = 0;
    virtual std::vector<std::string> loadCategories() = 0;
    virtual AppSettings loadSettings() = 0;

    virtual void saveProducts(const std::vector<Product>& products) = 0;
    virtual void saveHistory(const std::vector<Product>& history) = 0;
    virtual void saveCategories(const std::vector<std::string>& categories) = 0;
    virtual void saveSettings(const AppSettings& settings) = 0;
};

class CsvDataStore final : public IDataStore {
public:
    explicit CsvDataStore(std::filesystem::path dataDirectory);

    std::vector<Product> loadProducts() override;
    std::vector<Product> loadHistory() override;
    std::vector<std::string> loadCategories() override;
    AppSettings loadSettings() override;

    void saveProducts(const std::vector<Product>& products) override;
    void saveHistory(const std::vector<Product>& history) override;
    void saveCategories(const std::vector<std::string>& categories) override;
    void saveSettings(const AppSettings& settings) override;

private:
    std::filesystem::path dataDirectory_;

    std::filesystem::path productsPath() const;
    std::filesystem::path historyPath() const;
    std::filesystem::path categoriesPath() const;
    std::filesystem::path settingsPath() const;
};

}  // namespace expirybud
