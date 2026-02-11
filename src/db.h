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
