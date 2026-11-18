#include "expirybud/Storage.h"
#include "expirybud/Text.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace expirybud {

namespace {

const char* productHeader =
    "id,name,category,manufacture_date,expiry_date,quantity,unit,unit_price,date_added,outcome,outcome_date";

std::string csvEscape(const std::string& value) {
    if (value.find_first_of(",\"\r\n") == std::string::npos) {
        return value;
    }
    std::string escaped = "\"";
    for (char character : value) {
        if (character == '\"') {
            escaped += "\"\"";
        } else {
            escaped += character;
        }
    }
    return escaped + "\"";
}

std::vector<std::string> parseCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool quoted = false;

    for (std::size_t index = 0; index < line.size(); ++index) {
        const char character = line[index];
        if (character == '\"') {
            if (quoted && index + 1 < line.size() && line[index + 1] == '\"') {
                field += '\"';
                ++index;
            } else {
                quoted = !quoted;
            }
        } else if (character == ',' && !quoted) {
            fields.push_back(field);
            field.clear();
        } else {
            field += character;
        }
    }
    if (quoted) {
        throw std::runtime_error("Unclosed quoted CSV field");
    }
    fields.push_back(field);
    return fields;
}

double parseDouble(const std::string& value, const std::string& fieldName) {
    std::size_t parsed = 0;
    try {
        const double result = std::stod(value, &parsed);
        if (parsed != value.size()) {
            throw std::invalid_argument("trailing content");
        }
        return result;
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid " + fieldName + " value: " + value);
    }
}

Product productFromFields(const std::vector<std::string>& fields) {
    if (fields.size() != 11) {
        throw std::runtime_error("Expected 11 product fields, found " +
                                 std::to_string(fields.size()));
    }

    Product product;
    product.id = fields[0];
    product.name = fields[1];
    product.category = fields[2];
    if (!fields[3].empty()) {
        product.manufactureDate = Date::parse(fields[3]);
    }
    product.expiryDate = Date::parse(fields[4]);
    product.quantity = parseDouble(fields[5], "quantity");
    product.unit = fields[6];
    if (!fields[7].empty()) {
        product.unitPrice = parseDouble(fields[7], "unit price");
    }
    product.dateAdded = Date::parse(fields[8]);
    product.outcome = inventoryOutcomeFromString(fields[9]);
    if (!fields[10].empty()) {
        product.outcomeDate = Date::parse(fields[10]);
    }
    return product;
}

std::string productToCsv(const Product& product) {
    std::ostringstream output;
    output << csvEscape(product.id) << ',' << csvEscape(product.name) << ','
           << csvEscape(product.category) << ','
           << (product.manufactureDate ? product.manufactureDate->toString() : "") << ','
           << product.expiryDate.toString() << ',' << std::setprecision(15) << product.quantity
           << ',' << csvEscape(product.unit) << ',';
    if (product.unitPrice) {
        output << std::setprecision(15) << *product.unitPrice;
    }
    output << ',' << product.dateAdded.toString() << ',' << toString(product.outcome) << ',';
    if (product.outcomeDate) {
        output << product.outcomeDate->toString();
    }
    return output.str();
}

std::vector<Product> loadProductFile(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return {};
    }

    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Unable to read " + path.string());
    }

    std::vector<Product> products;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (lineNumber == 1 || trim(line).empty()) {
            continue;
        }
        try {
            products.push_back(productFromFields(parseCsvLine(line)));
        } catch (const std::exception& error) {
            throw std::runtime_error(path.filename().string() + " line " +
                                     std::to_string(lineNumber) + ": " + error.what());
        }
    }
    return products;
}

template <typename Writer>
void atomicWrite(const std::filesystem::path& path, Writer writer) {
    const std::filesystem::path temporary = path.string() + ".tmp";
    {
        std::ofstream output(temporary, std::ios::trunc);
        if (!output) {
            throw std::runtime_error("Unable to write " + temporary.string());
        }
        writer(output);
        output.flush();
        if (!output) {
            throw std::runtime_error("Failed while writing " + temporary.string());
        }
    }

    std::error_code error;
    std::filesystem::rename(temporary, path, error);
    if (error) {
        std::filesystem::remove(path, error);
        error.clear();
        std::filesystem::rename(temporary, path, error);
        if (error) {
            throw std::runtime_error("Unable to replace " + path.string());
        }
    }
}

void saveProductFile(const std::filesystem::path& path,
                     const std::vector<Product>& products) {
    atomicWrite(path, [&](std::ostream& output) {
        output << productHeader << '\n';
        for (const Product& product : products) {
            output << productToCsv(product) << '\n';
        }
    });
}

}  // namespace

CsvDataStore::CsvDataStore(std::filesystem::path dataDirectory)
    : dataDirectory_(std::move(dataDirectory)) {
    std::filesystem::create_directories(dataDirectory_);
}

std::vector<Product> CsvDataStore::loadProducts() {
    return loadProductFile(productsPath());
}

std::vector<Product> CsvDataStore::loadHistory() {
    return loadProductFile(historyPath());
}

std::vector<std::string> CsvDataStore::loadCategories() {
    if (!std::filesystem::exists(categoriesPath())) {
        return {"Food", "Medicine", "Cosmetics", "Dairy", "Grocery", "Household"};
    }

    std::ifstream input(categoriesPath());
    if (!input) {
        throw std::runtime_error("Unable to read " + categoriesPath().string());
    }
    std::vector<std::string> categories;
    std::string line;
    while (std::getline(input, line)) {
        const std::string category = trim(line);
        if (!category.empty()) {
            categories.push_back(category);
        }
    }
    return categories;
}

AppSettings CsvDataStore::loadSettings() {
    AppSettings settings;
    if (!std::filesystem::exists(settingsPath())) {
        return settings;
    }

    std::ifstream input(settingsPath());
    if (!input) {
        throw std::runtime_error("Unable to read " + settingsPath().string());
    }
    std::string line;
    while (std::getline(input, line)) {
        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }
        if (trim(line.substr(0, separator)) == "alert_threshold_days") {
            const std::string value = trim(line.substr(separator + 1));
            try {
                settings.alertThresholdDays = std::stoi(value);
            } catch (const std::exception&) {
                throw std::runtime_error("Invalid alert threshold in settings.txt");
            }
        }
    }
    if (settings.alertThresholdDays < 0 || settings.alertThresholdDays > 365) {
        throw std::runtime_error("Alert threshold must be between 0 and 365 days");
    }
    return settings;
}

void CsvDataStore::saveProducts(const std::vector<Product>& products) {
    saveProductFile(productsPath(), products);
}

void CsvDataStore::saveHistory(const std::vector<Product>& history) {
    saveProductFile(historyPath(), history);
}

void CsvDataStore::saveCategories(const std::vector<std::string>& categories) {
    atomicWrite(categoriesPath(), [&](std::ostream& output) {
        for (const std::string& category : categories) {
            output << category << '\n';
        }
    });
}

void CsvDataStore::saveSettings(const AppSettings& settings) {
    atomicWrite(settingsPath(), [&](std::ostream& output) {
        output << "alert_threshold_days=" << settings.alertThresholdDays << '\n';
    });
}

std::filesystem::path CsvDataStore::productsPath() const {
    return dataDirectory_ / "products.csv";
}

std::filesystem::path CsvDataStore::historyPath() const {
    return dataDirectory_ / "history.csv";
}

std::filesystem::path CsvDataStore::categoriesPath() const {
    return dataDirectory_ / "categories.csv";
}

std::filesystem::path CsvDataStore::settingsPath() const {
    return dataDirectory_ / "settings.txt";
}

}  // namespace expirybud
