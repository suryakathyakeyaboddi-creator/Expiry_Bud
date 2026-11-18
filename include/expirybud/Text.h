#pragma once

#include <string>

namespace expirybud {

std::string trim(const std::string& value);
std::string lowercase(std::string value);
bool equalsIgnoreCase(const std::string& left, const std::string& right);
bool containsIgnoreCase(const std::string& text, const std::string& query);

}  // namespace expirybud
