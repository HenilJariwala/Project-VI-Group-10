#pragma once

/**
 * @file timeutil.h
 * @brief Time parsing and formatting utilities.
 * @authors Everyone is an author baby this is a team effort
 *
 * Handles ISO-8601 UTC conversions.
 */

#include <chrono>
#include <string>

/**
 * @brief Time utility functions for ISO-8601 parsing and formatting.
 * @author Mohammad Aljabrery

 *
 * Provides helpers for converting between UTC strings
 * and std::chrono time points.
 */
namespace timeutil {

/**
     * @brief Parses an ISO-8601 UTC string into a time_point.
     * @author Mohammad Aljabrery
     *
     * Accepts formats like:
     * - "2026-02-11T01:29:13Z"
     * - "2026-02-11T01:29:13.123Z"
     *
     * @param iso Input UTC timestamp string.
     * @param out Output time_point if parsing succeeds.
     * @return True if parsing was successful.
     */
    bool parseIso8601Utc(const std::string& iso, std::chrono::system_clock::time_point& out);


/**
     * @brief Formats a UTC time_point into ISO-8601 string.
     * @author Mohammad Aljabrery

     *
     * Format: "YYYY-MM-DDTHH:MM:SSZ"
     *
     * @param tp Time point to format.
     * @return Formatted UTC string.
     */
    std::string formatIso8601Utc(const std::chrono::system_clock::time_point& tp);

}
