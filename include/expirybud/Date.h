#pragma once

#include <string>

namespace expirybud {

class Date {
public:
    Date();
    Date(int year, int month, int day);

    static Date parse(const std::string& value);
    static Date today();
    static bool isValid(int year, int month, int day);

    std::string toString() const;
    int daysUntil(const Date& other) const;

    int year() const { return year_; }
    int month() const { return month_; }
    int day() const { return day_; }

    bool operator==(const Date& other) const;
    bool operator!=(const Date& other) const { return !(*this == other); }
    bool operator<(const Date& other) const;
    bool operator<=(const Date& other) const { return *this < other || *this == other; }
    bool operator>(const Date& other) const { return other < *this; }
    bool operator>=(const Date& other) const { return !(*this < other); }

private:
    int year_;
    int month_;
    int day_;

    long serialDay() const;
};

}  // namespace expirybud
