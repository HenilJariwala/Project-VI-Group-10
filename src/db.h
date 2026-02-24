#pragma once

#include <sqlite3.h>
#include <string>
#include "crow_all.h"

class Db {
public:
    explicit Db(const std::string& path);
    ~Db();

    void initSchema(const std::string& schemaPath);
    void seedIfEmpty(const std::string& seedPath);

    // Lists
    crow::json::wvalue getAllFlights();
    crow::json::wvalue getAllPlanes();
    crow::json::wvalue getAllAirports();
    crow::json::wvalue getAllAirlines();

    // Pagination (forced 100 per page happens in main.cpp)
    // sort: "departure" or "gate" (anything else defaults to departure)
    crow::json::wvalue getFlightsPage(int limit, int offset,
                                  const std::string& sort,
                                  const std::string& search,
                                  const std::string& date);

    int getFlightsCount(const std::string& search,
                    const std::string& date);

    // CRUD
    int createFlight(int planeID, int airlineID,
                     int originAirportID, int destinationAirportID,
                     const std::string& gate,
                     int passengerCount, const std::string& departureTime);

    // Flight CRUD for update/delete flows
    // Returns true if the flight exists and populates `out` with raw Flight table fields.
    bool getFlightById(int flightID, crow::json::wvalue& out);

    // Returns true if a row was updated.
    bool updateFlight(int flightID,
                      int planeID,
                      int airlineID,
                      int originAirportID,
                      int destinationAirportID,
                      const std::string& gate,
                      int passengerCount,
                      const std::string& departureTime);

    // Returns true if a row was deleted.
    bool deleteFlight(int flightID);

private:
    sqlite3* db_{nullptr};

    int getTableCount(const std::string& tableName);
    void execSqlFile(const std::string& path);
};