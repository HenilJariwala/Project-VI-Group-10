#pragma once
#include <cmath>

namespace geo {

// returns great-circle distance in kilometers using the Haversine forumla 
double haversineKm(double lat1Deg, double lon1Deg, double lat2Deg, double lon2Deg);

// converts a distance and speed (km/h) to minutes (rounded up).
int durationMinutes(double distanceKm, int speedKmh);

// formats minutes as "Xh Ym".
const char* durationText(int minutes, char* buf, int bufSize);

}
