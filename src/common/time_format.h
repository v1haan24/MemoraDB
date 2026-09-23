#pragma once

#include <cstdint>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

inline std::string formatTimestamp(uint64_t timestampMs) {
    const std::time_t seconds =
        static_cast<std::time_t>(timestampMs / 1000ULL);
    const uint64_t millis = timestampMs % 1000ULL;

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &seconds);
#else
    localtime_r(&seconds, &tm);
#endif

    std::ostringstream out;
    out << std::setfill('0')
        << std::setw(4) << tm.tm_year + 1900 << '-'
        << std::setw(2) << tm.tm_mon + 1 << '-'
        << std::setw(2) << tm.tm_mday << ' '
        << std::setw(2) << tm.tm_hour << ':'
        << std::setw(2) << tm.tm_min << ':'
        << std::setw(2) << tm.tm_sec << '.'
        << std::setw(3) << millis;
    return out.str();
}
