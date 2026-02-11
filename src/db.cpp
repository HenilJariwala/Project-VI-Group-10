#include "db.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

//opens or makes the sqlite db
Db::Db(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }
    //enable the foreign key constraints
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
}

// destructor
// closes the sqlite connection when the Db object is destroyed
Db::~Db() {
    if (db_) sqlite3_close(db_);
}

// Return all flights
crow::json::wvalue Db::getAllFlights() {
    crow::json::wvalue flights = crow::json::wvalue::list();
    sqlite3_stmt* stmt;

    const char* sql = R"(
        SELECT
        f.flightID,
        f.gate,
        f.passengerCount,
        f.departureTime,

        p.model AS planeModel,
        p.speed AS planeSpeed,

        al.name AS airlineName,
        al.logoPath AS airlineLogo,

        oa.code AS originCode,
        da.code AS destCode,

        oc.name AS originCity,
        dc.name AS destCity,

        oc.latitude  AS originLat,
        oc.longitude AS originLon,
        dc.latitude  AS destLat,
        dc.longitude AS destLon

        FROM Flight f
        JOIN Plane p   ON f.planeID = p.planeID
        JOIN Airline al ON f.airlineID = al.airlineID
        JOIN Airport oa ON f.originAirportID = oa.airportID
        JOIN Airport da ON f.destinationAirportID = da.airportID
        JOIN Cities oc  ON oa.cityID = oc.cityID
        JOIN Cities dc  ON da.cityID = dc.cityID
        ORDER BY f.departureTime;
    )";

    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        auto& flight = flights[i];

        flight["flightID"] = sqlite3_column_int(stmt, 0);
        flight["gate"] = (const char*)sqlite3_column_text(stmt, 1);
        flight["passengers"] = sqlite3_column_int(stmt, 2);
        flight["departureTime"] = (const char*)sqlite3_column_text(stmt, 3);

        flight["plane"] = (const char*)sqlite3_column_text(stmt, 4);
        flight["planeSpeed"] = sqlite3_column_int(stmt, 5);

        flight["airline"]["name"] =
            (const char*)sqlite3_column_text(stmt, 6);
        flight["airline"]["logoPath"] =
            (const char*)sqlite3_column_text(stmt, 7);

        flight["origin"]["code"] =
            (const char*)sqlite3_column_text(stmt, 8);
        flight["destination"]["code"] =
            (const char*)sqlite3_column_text(stmt, 9);

        flight["origin"]["city"] =
            (const char*)sqlite3_column_text(stmt, 10);
        flight["destination"]["city"] =
            (const char*)sqlite3_column_text(stmt, 11);

        flight["origin"]["latitude"] =
            sqlite3_column_double(stmt, 12);
        flight["origin"]["longitude"] =
            sqlite3_column_double(stmt, 13);
        flight["destination"]["latitude"] =
            sqlite3_column_double(stmt, 14);
        flight["destination"]["longitude"] =
            sqlite3_column_double(stmt, 15);

        i++;
    }


    sqlite3_finalize(stmt);
    return flights;
}

// Returns all Plane rows as a Crow JSON list of objects.
crow::json::wvalue Db::getAllPlanes() {
    const char* sql = "SELECT planeID, model, speed, maxSeats FROM Plane ORDER BY model ASC;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare getAllPlanes");
    }

    // create a JSON list container to hold the result objects
    crow::json::wvalue arr = crow::json::wvalue::list();
    int i = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        crow::json::wvalue p;
        p["planeID"] = sqlite3_column_int(stmt, 0);
        p["model"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        p["speed"] = sqlite3_column_int(stmt, 2);
        p["maxSeats"] = sqlite3_column_int(stmt, 3);
        arr[i++] = std::move(p);
    }

    sqlite3_finalize(stmt);
    return arr;
}


// Returns airports with their city name as a Crow JSON list.
crow::json::wvalue Db::getAllAirports() {
    const char* sql =
        "SELECT a.airportID, a.code, c.name "
        "FROM Airport a JOIN Cities c ON a.cityID = c.cityID "
        "ORDER BY a.code ASC;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare getAllAirports");
    }

    crow::json::wvalue arr = crow::json::wvalue::list();
    int i = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        crow::json::wvalue a;
        a["airportID"] = sqlite3_column_int(stmt, 0);
        a["code"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        a["city"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        arr[i++] = std::move(a);
    }

    sqlite3_finalize(stmt);
    return arr;
}

//return the airliens
crow::json::wvalue Db::getAllAirlines() {
    const char* sql =
        "SELECT airlineID, name, logoPath "
        "FROM Airline ORDER BY name ASC;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare getAllAirlines");
    }

    crow::json::wvalue arr = crow::json::wvalue::list();
    int i = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        crow::json::wvalue a;
        a["airlineID"] = sqlite3_column_int(stmt, 0);
        a["name"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        a["logoPath"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        arr[i++] = std::move(a);
    }

    sqlite3_finalize(stmt);
    return arr;
}


// Reads an entire file into a std::string so we can read schema.sql and also the seed.sql
static std::string readWholeFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) return "";
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// Executes an entire SQL file - so creating tables and inserting stuff from ther seed.sql
void Db::execSqlFile(const std::string& path) {
    auto sql = readWholeFile(path);
    if (sql.empty()) {
        throw std::runtime_error("Could not open SQL file: " + path);
    }

    char* err = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "Unknown SQL error";
        sqlite3_free(err);
        throw std::runtime_error(msg);
    }
}


// Runs schema.sql to ensure tables exist
void Db::initSchema(const std::string& schemaPath) {
    execSqlFile(schemaPath);
}


// Returns COUNT(*) for a given table. used to see if the db needs seeding
int Db::getTableCount(const std::string& tableName) {
    std::string sql = "SELECT COUNT(*) FROM " + tableName + ";";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare COUNT(*) for " + tableName);
    }

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}


// Seeds baseline tables (Cities, Airport, Plane) from seed.sql
void Db::seedIfEmpty(const std::string& seedPath) {
    int cities = getTableCount("Cities");
    int airports = getTableCount("Airport");
    int planes = getTableCount("Plane");

    if (cities == 0 && airports == 0 && planes == 0) {
        execSqlFile(seedPath);
    }
}


// Inserts a new flight row  in the db and returns the generated primary key (flightID)
int Db::createFlight(int planeID, int airlineID,
                     int originAirportID, int destinationAirportID,
                     const std::string& gate,
                     int passengerCount, const std::string& departureTime) {
    const char* sql =
        "INSERT INTO Flight(planeID, airlineID, originAirportID, destinationAirportID, gate, passengerCount, departureTime) "
        "VALUES(?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare createFlight");
    }

    sqlite3_bind_int(stmt, 1, planeID);
    sqlite3_bind_int(stmt, 2, airlineID);
    sqlite3_bind_int(stmt, 3, originAirportID);
    sqlite3_bind_int(stmt, 4, destinationAirportID);
    sqlite3_bind_text(stmt, 5, gate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, passengerCount);
    sqlite3_bind_text(stmt, 7, departureTime.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error("Failed to INSERT flight");
    }

    sqlite3_finalize(stmt);
    return static_cast<int>(sqlite3_last_insert_rowid(db_));
}
