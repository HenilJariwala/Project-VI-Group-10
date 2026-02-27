#pragma once

/**
 * @file db.h
 * @brief Database interface for Flight Logger.
 * @authors Everyone is an author baby this is a team effort
 *
 * Declares the Db class which wraps SQLite operations,
 * pagination, and flight CRUD logic.
 */

#include <sqlite3.h>
#include <string>
#include "crow_all.h"

/**
 * @brief SQLite database wrapper for Flight Logger.
 *
 * Handles schema setup, seed data, queries, pagination, and flight CRUD.
 */
class Db {
public:
    /**
     * @brief Opens or creates the database file.
     * @param path Path to the SQLite file.
     */    
explicit Db(const std::string& path);
    /**
     * @brief Closes the database connection.
     */
    ~Db();


    /**
     * @brief Runs the schema SQL file.
     * @param schemaPath Path to schema.sql.
     */
    void initSchema(const std::string& schemaPath);
    
    /**
     * @brief Seeds baseline tables if empty.
     * @param seedPath Path to seed.sql.
     */
    void seedIfEmpty(const std::string& seedPath);

    // Lists
    /** @brief Returns all flights (joined with related tables). */
    crow::json::wvalue getAllFlights();

    /** @brief Returns all planes. */
    crow::json::wvalue getAllPlanes();

    /** @brief Returns all airports. */
    crow::json::wvalue getAllAirports();

    /** @brief Returns all airlines. */
    crow::json::wvalue getAllAirlines();

    /**
     * @brief Returns one page of flights with optional filters.
     * @param limit Max rows to return.
     * @param offset Rows to skip.
     * @param sort Sort key ("departure" or "gate").
     * @param search Optional search string (empty for none).
     * @param date Optional date filter (empty for none).
     */
    crow::json::wvalue getFlightsPage(int limit, int offset,
                                  const std::string& sort,
                                  const std::string& search,
                                  const std::string& date);


    /**
     * @brief Returns total flights matching filters.
     * @param search Optional search string.
     * @param date Optional date filter.
     * @return Total matching rows.
     */
    int getFlightsCount(const std::string& search,
                    const std::string& date);

    
    /**
     * @brief Inserts a new flight.
     * @return New flightID.
     */
    int createFlight(int planeID, int airlineID,
                     int originAirportID, int destinationAirportID,
                     const std::string& gate,
                     int passengerCount, const std::string& departureTime);

    
                     

    /**
     * @brief Gets a flight by ID (raw Flight table fields).
     * @param flightID Flight ID.
     * @param out Output JSON object.
     * @return True if found.
     */
    bool getFlightById(int flightID, crow::json::wvalue& out);

    /**
     * @brief Updates a flight.
     * @return True if a row was updated.
     */
    bool updateFlight(int flightID,
                      int planeID,
                      int airlineID,
                      int originAirportID,
                      int destinationAirportID,
                      const std::string& gate,
                      int passengerCount,
                      const std::string& departureTime);

    
                      
    /**
     * @brief Deletes a flight.
     * @return True if a row was deleted.
     */
    bool deleteFlight(int flightID);

private:
    sqlite3* db_{nullptr};

    /** @brief Returns COUNT(*) for a table. */
    int getTableCount(const std::string& tableName);

    /** @brief Executes SQL statements from a file. */
    void execSqlFile(const std::string& path);
};