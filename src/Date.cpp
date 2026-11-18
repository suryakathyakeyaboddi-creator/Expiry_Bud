#include "expirybud/Date.h"

#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace expirybud {

namespace {

bool isLeapYear(int year) {
    return year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);
}

int daysInMonth(int year, int month) {
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year)) {
        return 29;
    }
    return days[month - 1];
}

}  // namespace

Date::Date() : year_(1970), month_(1), day_(1) {}

Date::Date(int year, int month, int day) : year_(year), month_(month), day_(day) {
    if (!isValid(year, month, day)) {
        throw std::invalid_argument("Invalid date");
    }
}

Date Date::parse(const std::string& value) {
    std::istringstream input(value);
    int day = 0;
    int month = 0;
    int year = 0;
    char firstSeparator = '\0';
    char secondSeparator = '\0';
    char extra = '\0';

    if (!(input >> day >> firstSeparator >> month >> secondSeparator >> year) ||
        firstSeparator != '-' || secondSeparator != '-' || (input >> extra)) {
        throw std::invalid_argument("Date must use DD-MM-YYYY format");
    }
    return Date(year, month, day);
}

Date Date::today() {
    const std::time_t current = std::time(nullptr);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &current);
#else
    localtime_r(&current, &local);
#endif
    return Date(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
}

bool Date::isValid(int year, int month, int day) {
    return year >= 1900 && year <= 9999 && month >= 1 && month <= 12 &&
           day >= 1 && day <= daysInMonth(year, month);
}

std::string Date::toString() const {
    std::ostringstream output;
    output << std::setfill('0') << std::setw(2) << day_ << '-' << std::setw(2) << month_
           << '-' << std::setw(4) << year_;
    return output.str();
}

int Date::daysUntil(const Date& other) const {
    return static_cast<int>(other.serialDay() - serialDay());
}

bool Date::operator==(const Date& other) const {
    return std::tie(year_, month_, day_) == std::tie(other.year_, other.month_, other.day_);
}

bool Date::operator<(const Date& other) const {
    return std::tie(year_, month_, day_) < std::tie(other.year_, other.month_, other.day_);
}

long Date::serialDay() const {
    int adjustedYear = year_ - (month_ <= 2 ? 1 : 0);
    const int era = (adjustedYear >= 0 ? adjustedYear : adjustedYear - 399) / 400;
    const unsigned yearOfEra = static_cast<unsigned>(adjustedYear - era * 400);
    const unsigned adjustedMonth = static_cast<unsigned>(month_ + (month_ > 2 ? -3 : 9));
    const unsigned dayOfYear = (153 * adjustedMonth + 2) / 5 +
                               static_cast<unsigned>(day_ - 1);
    const unsigned dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    return static_cast<long>(era) * 146097L + static_cast<long>(dayOfEra);
}

}  // namespace expirybud
