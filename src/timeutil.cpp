#include "timeutil.h"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace timeutil {

static std::string normalizeIso(const std::string& iso) {
    // Keep only up to seconds, remove milliseconds and trailing Z if present.
    // Example: 2026-02-11T01:29:13.123Z -> 2026-02-11T01:29:13
    std::string s = iso;

    // Drop trailing 'Z'
    if (!s.empty() && s.back() == 'Z') s.pop_back();

    // Drop fractional seconds
    auto dot = s.find('.');
    if (dot != std::string::npos) s = s.substr(0, dot);

    return s;
}

bool parseIso8601Utc(const std::string& iso, std::chrono::system_clock::time_point& out) {
    const std::string s = normalizeIso(iso);

    std::tm tm{};
    std::istringstream ss(s);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail()) return false;

    // Interpret as UTC:
#if defined(_WIN32)
    std::time_t t = _mkgmtime(&tm);
#else
    std::time_t t = timegm(&tm);
#endif
    out = std::chrono::system_clock::from_time_t(t);
    return true;
}

std::string formatIso8601Utc(const std::chrono::system_clock::time_point& tp) {
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif

    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

}
