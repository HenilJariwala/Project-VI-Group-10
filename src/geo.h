#pragma once

/**
 * @file geo.h
 * @brief Geographic utility functions.
 * @authors Everyone is an author baby this is a team effort
 *
 * Provides distance and duration calculations.
 */

#include <cmath>

/**
 * @brief Geographic utility functions.
 *
 * Provides distance and flight duration calculations.
 */
namespace geo {

// returns great-circle distance in kilometers using the Haversine forumla 
/**
     * @brief Computes great-circle distance using the Haversine formula.
     * @author Mohammad Aljabrery
     * @param lat1Deg Latitude of origin in degrees.
     * @param lon1Deg Longitude of origin in degrees.
     * @param lat2Deg Latitude of destination in degrees.
     * @param lon2Deg Longitude of destination in degrees.
     * @return Distance in kilometers.
     */
double haversineKm(double lat1Deg, double lon1Deg, double lat2Deg, double lon2Deg);

// converts a distance and speed (km/h) to minutes (rounded up).
/**
     * @brief Converts distance and speed to duration in minutes.
     * @author Mohammad Aljabrery
     * @param distanceKm Distance in kilometers.
     * @param speedKmh Aircraft speed in km/h.
     * @return Duration in minutes (rounded up).
     */
int durationMinutes(double distanceKm, int speedKmh);




// formats minutes as "Xh Ym".
/**
     * @brief Formats duration as "Xh Ym".
     * @author Mohammad Aljabrery
     * @param minutes Duration in minutes.
     * @param buf Output buffer.
     * @param bufSize Size of buffer.
     * @return Pointer to formatted buffer.
     */
const char* durationText(int minutes, char* buf, int bufSize);

}
