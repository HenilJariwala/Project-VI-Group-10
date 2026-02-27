/**
 * @file main.cpp
 * @brief Entry point and HTTP route definitions.
 * @authors Everyone is an author baby this is a team effort
 *
 * Initializes the database and configures all API endpoints.
 */


#include "crow_all.h"
#include "db.h"
#include "geo.h"
#include "timeutil.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <map>

/**
 * @brief In-memory flight model used to enrich API responses.
 *
 * Built from database query results and used to compute
 * status, distance, duration, and arrival time.
 */
struct FlightItem {
    int flightID;
    std::string airlineName;
    std::string airlineLogoPath;
    std::string plane;
    int planeSpeed =0;
    std::string arrivalTime;
    double distanceKm =0.0;
    int durationMinutes = 0;
    std::string gate;
    int passengers;
    std::string departureTime;
    
    struct Airport {
        std::string city;
        std::string code;
        double latitude;
        double longitude;
    } origin, destination;
};

/**
 * @brief Reads an entire file into a string.
 * @param path File path.
 * @return File contents, or empty string if open fails.
 */
static std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::in);
    if (!in) return "";
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

/**
 * @brief Serves a static file as an HTTP response.
 * @param path File path on disk.
 * @param contentType Value for the Content-Type header.
 * @return 200 response with file contents, or 404 if missing.
 */
static crow::response serveFile(const std::string& path, const std::string& contentType) {
    auto body = readFile(path);
    if (body.empty()) return crow::response(404, "Not Found");
    crow::response res{200, body};
    res.set_header("Content-Type", contentType);
    return res;
}

/**
 * @brief Converts a JSON list of flights into FlightItem objects.
 * @param flightsJson JSON list returned from the database layer.
 * @return Vector of FlightItem records.
 */
std::vector<FlightItem> jsonToFlights(const crow::json::rvalue& flightsJson) {
    std::vector<FlightItem> result;

    for (size_t i = 0; i < flightsJson.size(); ++i) {
        const auto& f = flightsJson[i];
        FlightItem item;

        item.flightID      = f["flightID"].i();
        item.airlineName   = f["airline"]["name"].s();
        item.airlineLogoPath = f["airline"]["logoPath"].s();
        item.plane         = f["plane"].s();
        item.planeSpeed = f["planeSpeed"].i();
        item.gate          = f["gate"].s();
        item.passengers    = f["passengers"].i();
        item.departureTime = f["departureTime"].s();

        item.origin.city      = f["origin"]["city"].s();
        item.origin.code      = f["origin"]["code"].s();
        item.origin.latitude  = f["origin"]["latitude"].d();
        item.origin.longitude = f["origin"]["longitude"].d();

        item.destination.city      = f["destination"]["city"].s();
        item.destination.code      = f["destination"]["code"].s();
        item.destination.latitude  = f["destination"]["latitude"].d();
        item.destination.longitude = f["destination"]["longitude"].d();

        result.push_back(item);
    }
    return result;
}

/**
 * @brief Application entry point. DUHHHHHHHH
 *
 * Initializes the database, configures API routes, and starts the HTTP server.
 */
int main() {
    // init db
    std::filesystem::create_directories("/app/runtime_db");
    Db db("/app/runtime_db/flights.db");
    db.initSchema("/app/src/schema.sql");
    db.seedIfEmpty("/app/src/seed.sql");

    crow::SimpleApp app;

    /**
    * @brief GET /
    * @brief Redirects to the flights page.
    */
    CROW_ROUTE(app, "/")([]{
        crow::response res;
        res.code = 302;
        res.set_header("Location", "/pages/flights.html");
        return res;
    });

    /**
     * @brief GET /api/flights
     * @brief Returns a paginated flight list with optional filtering and sorting.
     *
     * Query params:
     * - search: optional keyword filter
     * - sort: departure | gate | status
     * - date: optional date filter
     * - page: page number (1-based)
     */
    CROW_ROUTE(app, "/api/flights").methods(crow::HTTPMethod::GET)
    ([&db](const crow::request& req){
        std::string search = req.url_params.get("search") ? req.url_params.get("search") : "";
        std::string sort   = req.url_params.get("sort") ? req.url_params.get("sort") : "status";
        std::string dateStr = req.url_params.get("date") ? req.url_params.get("date") : "";
        // normalize datetime-local format (YYYY-MM-DDTHH:MM -> YYYY-MM-DD HH:MM)
        if (!dateStr.empty()) {
            std::replace(dateStr.begin(), dateStr.end(), 'T', ' ');
        }

        // normalize sort (avoid weird values)
        if (sort != "departure" && sort != "gate" && sort != "status")
            sort = "status";

        // Pagination (forced 100 per page)
        int page = 1;
        int size = 100;

        if (req.url_params.get("page"))
            page = std::max(1, std::atoi(req.url_params.get("page")));

        int offset = (page - 1) * size;

        // Pull only 100 rows from DB (sorted by departure/gate in SQL)
       auto flightsWvalue = db.getFlightsPage(size, offset, sort, search, dateStr);

        // Convert to rvalue -> vector<FlightItem>
        std::string jsonStr = flightsWvalue.dump();
        auto flightsRvalue = crow::json::load(jsonStr);
        auto flights = jsonToFlights(flightsRvalue);

        // Search filter 
        

        // Helper functions for status and progress 
        auto now = std::chrono::system_clock::now();

        auto getStatus = [&](const FlightItem& f) -> std::pair<std::string, std::string> {
            std::tm depTm = {};
            std::istringstream ds(f.departureTime);
            ds >> std::get_time(&depTm, "%Y-%m-%dT%H:%M:%S");
            auto dep = std::chrono::system_clock::from_time_t(std::mktime(&depTm));
            auto diff = std::chrono::duration_cast<std::chrono::minutes>(dep - now).count();

            if (diff < 0) return {"departed", "DEPARTED"};
            if (diff < 30) return {"boarding", "BOARDING"};
            return {"ontime", "ON TIME"};
        };

        auto getProgress = [&](const FlightItem& f) -> double {
            std::tm depTm = {};
            std::istringstream ds(f.departureTime);
            ds >> std::get_time(&depTm, "%Y-%m-%dT%H:%M:%S");
            auto dep = std::chrono::system_clock::from_time_t(std::mktime(&depTm));

            // Progress: 0 at (departure - 30 min), 1.0 at departure
            auto boardingStart = dep - std::chrono::minutes(30);
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - boardingStart).count();
            auto total = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::minutes(30)).count();

            double progress = static_cast<double>(elapsed) / total;
            return std::min(std::max(progress, 0.0), 1.0);
        };

        // Sorting
        if (sort != "departure" && sort != "gate") {
            std::map<std::string,int> statusOrder = {{"boarding",0}, {"ontime",1}, {"departed",2}};
            std::sort(flights.begin(), flights.end(), [&](const FlightItem& a, const FlightItem& b){
                auto statusA = getStatus(a).first;
                auto statusB = getStatus(b).first;
                return statusOrder[statusA] < statusOrder[statusB];
            });
        }

        // Convert to JSON with status and progress
        crow::json::wvalue out;
        std::vector<crow::json::wvalue> flightsList;

        for (auto& f : flights) {
            crow::json::wvalue j;

            j["flightID"] = f.flightID;
            j["airline"]["name"] = f.airlineName;
            j["airline"]["logoPath"] = f.airlineLogoPath;
            j["plane"] = f.plane;
            j["gate"] = f.gate;
            j["passengers"] = f.passengers;
            j["departureTime"] = f.departureTime;

            // Add status and progress
            auto [statusClass, statusText] = getStatus(f);
            j["status"]["class"] = statusClass;
            j["status"]["text"] = statusText;
            j["progress"] = getProgress(f);

            j["origin"]["city"] = f.origin.city;
            j["origin"]["code"] = f.origin.code;
            j["origin"]["latitude"] = f.origin.latitude;
            j["origin"]["longitude"] = f.origin.longitude;

            j["destination"]["city"] = f.destination.city;
            j["destination"]["code"] = f.destination.code;
            j["destination"]["latitude"] = f.destination.latitude;
            j["destination"]["longitude"] = f.destination.longitude;


            f.distanceKm = geo::haversineKm(
                f.origin.latitude, f.origin.longitude,
                f.destination.latitude, f.destination.longitude
            );
            f.durationMinutes = geo::durationMinutes(f.distanceKm, f.planeSpeed);

            std::chrono::system_clock::time_point depTp;
            if (timeutil::parseIso8601Utc(f.departureTime, depTp)) {
                auto arrTp = depTp + std::chrono::minutes(f.durationMinutes);
                f.arrivalTime = timeutil::formatIso8601Utc(arrTp);
            } else {
                f.arrivalTime = "";
            }

            int h = f.durationMinutes / 60;
            int m = f.durationMinutes % 60;
            std::string durationText = std::to_string(h) + "h " + std::to_string(m) + "m";

            j["distanceKm"] = f.distanceKm;
            j["durationMinutes"] = f.durationMinutes;
            j["durationText"] = durationText;
            j["arrivalTime"] = f.arrivalTime;

            flightsList.push_back(std::move(j));
        }

        // Pagination metadata (NOTE: total is unfiltered for now)
        int total = db.getFlightsCount(search, dateStr);
        out["page"] = page;
        out["size"] = size;
        out["total"] = total;
        out["totalPages"] = (total + size - 1) / size;

        out["flights"] = std::move(flightsList);

        crow::response res;
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.body = out.dump();
        return res;
    });
    // assets routes (images, gifs)
    CROW_ROUTE(app, "/assets/<string>")([](const std::string& file){
        return serveFile("/app/public/assets/" + file, "image/gif");
    });

    //assets route for the logos
    CROW_ROUTE(app, "/assets/<string>/<string>")
    ([](const std::string& folder, const std::string& file){
        return serveFile("/app/public/assets/" + folder + "/" + file, "image/png");
    });


    // pages routes
    CROW_ROUTE(app, "/pages/<string>")([](const std::string& file){
        return serveFile("/app/public/pages/" + file, "text/html; charset=utf-8");
    });

    // styles routes
    CROW_ROUTE(app, "/styles/<string>")([](const std::string& file){
        return serveFile("/app/public/styles/" + file, "text/css; charset=utf-8");
    });

    //scripts routes
    CROW_ROUTE(app, "/scripts/<string>")([](const std::string& file){
        return serveFile("/app/public/scripts/" + file, "application/javascript; charset=utf-8");
    });

    CROW_ROUTE(app, "/api/planes").methods(crow::HTTPMethod::GET)
    ([&db]{
        crow::json::wvalue out;
        out["planes"] = db.getAllPlanes();
        crow::response res{200, out.dump()};
        res.set_header("Content-Type", "application/json");
        return res;
    });

    CROW_ROUTE(app, "/api/airports").methods(crow::HTTPMethod::GET)
    ([&db]{
        crow::json::wvalue out;
        out["airports"] = db.getAllAirports();
        crow::response res{200, out.dump()};
        res.set_header("Content-Type", "application/json");
        return res;
    });

    CROW_ROUTE(app, "/api/airlines").methods(crow::HTTPMethod::GET)
    ([&db]{
        crow::json::wvalue out;
        out["airlines"] = db.getAllAirlines();
        crow::response res{200, out.dump()};
        res.set_header("Content-Type", "application/json");
        return res;
    });
    
    /**
     * @brief POST /api/flights
     * @brief Creates a new flight record.
     */
    CROW_ROUTE(app, "/api/flights").methods(crow::HTTPMethod::POST)
    ([&db](const crow::request& req){
        auto body = crow::json::load(req.body);
        if (!body) return crow::response{400, "Invalid JSON"};

        const char* fields[] = {
            "planeID","originAirportID","destinationAirportID",
            "airlineID","gate","passengerCount","departureTime"
        };

        for (auto f : fields) {
            if (!body.has(f)) return crow::response{400, std::string("Missing field: ") + f};
        }

        if (body["originAirportID"].i() == body["destinationAirportID"].i()) {
            return crow::response{400, "originAirportID and destinationAirportID must be different"};
        }

        try {
            int id = db.createFlight(
                body["planeID"].i(),
                body["airlineID"].i(),
                body["originAirportID"].i(),
                body["destinationAirportID"].i(),
                body["gate"].s(),
                body["passengerCount"].i(),
                body["departureTime"].s()
            );

            crow::json::wvalue out;
            out["message"] = "Flight created";
            out["flightID"] = id;

            crow::response res;
            res.code = 201;
            res.set_header("Content-Type", "application/json");
            res.body = out.dump();;
            return res;
        } catch (const std::exception& e) {
            return crow::response{500, e.what()};
        }
    });


    /**
     * @brief GET /api/flights/{id}
     * @brief Returns a single flight by ID.
     */
    CROW_ROUTE(app, "/api/flights/<int>").methods(crow::HTTPMethod::GET)
    ([&db](int flightID){
        try {
            crow::json::wvalue out;
            if (!db.getFlightById(flightID, out)) {
                return crow::response{404, "Flight not found"};
            }

            crow::response res;
            res.code = 200;
            res.set_header("Content-Type", "application/json");
            res.body = out.dump();
            return res;
        } catch (const std::exception& e) {
            return crow::response{500, e.what()};
        }
    });

    /** @brief PUT /api/flights/{id} @brief Replaces a flight record. */
    CROW_ROUTE(app, "/api/flights/<int>").methods(crow::HTTPMethod::PUT)
    ([&db](const crow::request& req, int flightID){
        auto body = crow::json::load(req.body);
        if (!body) return crow::response{400, "Invalid JSON"};

        const char* fields[] = {
            "planeID","originAirportID","destinationAirportID",
            "airlineID","gate","passengerCount","departureTime"
        };


        for (auto f : fields) {
            if (!body.has(f)) return crow::response{400, std::string("Missing field: ") + f};
        }

        if (body["originAirportID"].i() == body["destinationAirportID"].i()) {
            return crow::response{400, "originAirportID and destinationAirportID must be different"};
        }

        try {
            crow::json::wvalue existing;
            if (!db.getFlightById(flightID, existing)) {
                return crow::response{404, "Flight not found"};
            }

            bool ok = db.updateFlight(
                flightID,
                body["planeID"].i(),
                body["airlineID"].i(),
                body["originAirportID"].i(),
                body["destinationAirportID"].i(),
                body["gate"].s(),
                body["passengerCount"].i(),
                body["departureTime"].s()
            );

            if (!ok) return crow::response{404, "Flight not found"};

            crow::json::wvalue out;
            out["message"] = "Flight updated";
            out["flightID"] = flightID;

            crow::response res;
            res.code = 200;
            res.set_header("Content-Type", "application/json");
            res.body = out.dump();
            return res;
        } catch (const std::exception& e) {
            return crow::response{500, e.what()};
        }
    });

    /** @brief PATCH /api/flights/{id} @brief Partially updates a flight record. */
    CROW_ROUTE(app, "/api/flights/<int>").methods(crow::HTTPMethod::PATCH)
    ([&db](const crow::request& req, int flightID){
        auto body = crow::json::load(req.body);
        if (!body) return crow::response{400, "Invalid JSON"};

        try {
            // Fetch existing flight (raw Flight table fields)
            crow::json::wvalue existingW;
            if (!db.getFlightById(flightID, existingW)) {
                return crow::response{404, "Flight not found"};
            }

            // Convert wvalue -> rvalue so we can use .i() / .s()
            auto existing = crow::json::load(existingW.dump());
            if (!existing) {
                return crow::response{500, "Failed to parse existing flight JSON"};
            }

            // Merge: body overrides existing for any provided field
            int planeID = body.has("planeID")
                ? body["planeID"].i()
                : existing["planeID"].i();

            int airlineID = body.has("airlineID")
                ? body["airlineID"].i()
                : existing["airlineID"].i();

            int originAirportID = body.has("originAirportID")
                ? body["originAirportID"].i()
                : existing["originAirportID"].i();

            int destinationAirportID = body.has("destinationAirportID")
                ? body["destinationAirportID"].i()
                : existing["destinationAirportID"].i();

            std::string gate = body.has("gate")
                ? std::string(body["gate"].s())
                : std::string(existing["gate"].s());

            int passengerCount = body.has("passengerCount")
                ? body["passengerCount"].i()
                : existing["passengerCount"].i();

            std::string departureTime = body.has("departureTime")
                ? std::string(body["departureTime"].s())
                : std::string(existing["departureTime"].s());

            // Validation
            if (originAirportID == destinationAirportID) {
                return crow::response{400, "originAirportID and destinationAirportID must be different"};
            }

            // Apply update
            bool ok = db.updateFlight(
                flightID,
                planeID,
                airlineID,
                originAirportID,
                destinationAirportID,
                gate,
                passengerCount,
                departureTime
            );

            if (!ok) return crow::response{404, "Flight not found"};

            crow::json::wvalue out;
            out["message"] = "Flight patched";
            out["flightID"] = flightID;

            crow::response res;
            res.code = 200;
            res.set_header("Content-Type", "application/json");
            res.body = out.dump();
            return res;

        } catch (const std::exception& e) {
            return crow::response{500, e.what()};
        }
    });



    /** @brief DELETE /api/flights/{id} @brief Deletes a flight record. */
    CROW_ROUTE(app, "/api/flights/<int>").methods(crow::HTTPMethod::DELETE)
    ([&db](int flightID){
        try {
            bool ok = db.deleteFlight(flightID);
            if (!ok) return crow::response{404, "Flight not found"};

            crow::json::wvalue out;
            out["message"] = "Flight deleted";
            out["flightID"] = flightID;

            crow::response res;
            res.code = 200;
            res.set_header("Content-Type", "application/json");
            res.body = out.dump();
            return res;
        } catch (const std::exception& e) {
            return crow::response{500, e.what()};
        }
    });

    // OPTIONS /api/flights
    CROW_ROUTE(app, "/api/flights").methods(crow::HTTPMethod::OPTIONS)
    ([]{
        std::cout << "\nOPTIONS /api/flights iS HIT \n" << std::endl;
        crow::response res;
        res.code = 204; 
        res.set_header("Allow", "GET,POST,OPTIONS");
        return res;
    });

    // OPTIONS /api/flights/<id>
    CROW_ROUTE(app, "/api/flights/<int>").methods(crow::HTTPMethod::OPTIONS)
    ([](int){
        crow::response res;
        res.code = 204; 
        res.set_header("Allow", "GET,PUT,PATCH,DELETE,OPTIONS");
        return res;
    });

    //run
    app.port(18080).multithreaded().run();
    return 0;
}
