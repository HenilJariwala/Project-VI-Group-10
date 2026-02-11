#include "geo.h"
#include <algorithm>
#include <cstdio>

namespace geo {

static double degToRad(double deg) {
    return deg * (3.14159265358979323846 / 180.0);
}

double haversineKm(double lat1Deg, double lon1Deg, double lat2Deg, double lon2Deg) {
    const double R = 6371.0; // earth's radius in km
    const double lat1 = degToRad(lat1Deg);
    const double lat2 = degToRad(lat2Deg);
    const double dLat = degToRad(lat2Deg - lat1Deg);
    const double dLon = degToRad(lon2Deg - lon1Deg);

    const double a =
        std::sin(dLat / 2) * std::sin(dLat / 2) +
        std::cos(lat1) * std::cos(lat2) *
        std::sin(dLon / 2) * std::sin(dLon / 2);

    const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return R * c;
}

int durationMinutes(double distanceKm, int speedKmh) {
    if (speedKmh <= 0) return 0;
    const double hours = distanceKm / static_cast<double>(speedKmh);
    
    // round up so we don't show "0 minutes" for short flights.
    const int minutes = static_cast<int>(std::ceil(hours * 60.0));
    return std::max(0, minutes);
}

const char* durationText(int minutes, char* buf, int bufSize) {
    const int h = minutes / 60;
    const int m = minutes % 60;
    std::snprintf(buf, bufSize, "%dh %dm", h, m);
    return buf;
}

}
