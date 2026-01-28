#pragma once
#include <sqlite3.h>
#include <string>
#include "crow_all.h"

class Db {
public:
    explicit Db(const std::string& path);
    ~Db();

    void initSchema(const std::string& schemaPath);

    //seed baseline data if the tables are empty
    void seedIfEmpty(const std::string& seedPath);

    // Flights
    crow::json::wvalue getAllFlights();

    crow::json::wvalue getAllPlanes();
    crow::json::wvalue getAllAirports();
    int createFlight(int planeID, int originAirportID, int destinationAirportID,
                 const std::string& airline, const std::string& gate,
                 int passengerCount, const std::string& departureTime);

private:
    sqlite3* db_{nullptr};

    int getTableCount(const std::string& tableName);
    void execSqlFile(const std::string& path);
    
};
