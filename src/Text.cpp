#include "expirybud/Text.h"

#include <algorithm>
#include <cctype>

namespace expirybud {

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

bool equalsIgnoreCase(const std::string& left, const std::string& right) {
    return lowercase(trim(left)) == lowercase(trim(right));
}

bool containsIgnoreCase(const std::string& text, const std::string& query) {
    return lowercase(text).find(lowercase(query)) != std::string::npos;
}

}  // namespace expirybud
