#pragma once
#include <chrono>
#include <string>

namespace timeutil {

// Parses ISO like "2026-02-11T01:29:13.123Z" or "2026-02-11T01:29:13Z"
// Returns true if parse worked.
bool parseIso8601Utc(const std::string& iso, std::chrono::system_clock::time_point& out);

// Formats a UTC time_point into "YYYY-MM-DDTHH:MM:SSZ"
std::string formatIso8601Utc(const std::chrono::system_clock::time_point& tp);

}
