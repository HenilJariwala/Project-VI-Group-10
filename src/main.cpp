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

static std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::in);
    if (!in) return "";
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static crow::response serveFile(const std::string& path, const std::string& contentType) {
    auto body = readFile(path);
    if (body.empty()) return crow::response(404, "Not Found");
    crow::response res{200, body};
    res.set_header("Content-Type", contentType);
    return res;
}

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


int main() {
    // init db
    std::filesystem::create_directories("/app/runtime_db");
    Db db("/app/runtime_db/flights.db");
    db.initSchema("/app/src/schema.sql");
    db.seedIfEmpty("/app/src/seed.sql");

    crow::SimpleApp app;

    // flights is the main page
    CROW_ROUTE(app, "/")([]{
        crow::response res;
        res.code = 302;
        res.set_header("Location", "/pages/flights.html");
        return res;
    });

    CROW_ROUTE(app, "/api/flights").methods(crow::HTTPMethod::GET)
    ([&db](const crow::request& req){
        std::string search = req.url_params.get("search") ? req.url_params.get("search") : "";
        std::string sort   = req.url_params.get("sort") ? req.url_params.get("sort") : "status";
        std::string dateStr = req.url_params.get("date") ? req.url_params.get("date") : "";

        auto flightsWvalue = db.getAllFlights();
        std::string jsonStr = flightsWvalue.dump();
        auto flightsRvalue = crow::json::load(jsonStr);
        auto flights = jsonToFlights(flightsRvalue);

        // Search filter 
        if (!search.empty()) {
            std::string lowerSearch = search;
            std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);

            flights.erase(
                std::remove_if(flights.begin(), flights.end(), [&](const FlightItem& f) {
                    auto check = [&](const std::string& s) {
                        std::string tmp = s;
                        std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
                        return tmp.find(lowerSearch) == std::string::npos;
                    };
                    return check(f.airlineName) && check(f.origin.city) && check(f.origin.code) &&
                        check(f.destination.city) && check(f.destination.code) &&
                        check(f.gate) && check(f.plane);
                }),
                flights.end()
            );
        }

        // Date filter ±24 hours 
        if (!dateStr.empty()) {
            std::tm tm = {};
            std::istringstream ss(dateStr);
            ss >> std::get_time(&tm, "%Y-%m-%d");
            auto selectedTime = std::chrono::system_clock::from_time_t(std::mktime(&tm));

            flights.erase(
                std::remove_if(flights.begin(), flights.end(), [&](const FlightItem& f) {
                    std::tm depTm = {};
                    std::istringstream ds(f.departureTime);
                    ds >> std::get_time(&depTm, "%Y-%m-%dT%H:%M:%S");
                    auto depTime = std::chrono::system_clock::from_time_t(std::mktime(&depTm));
                    auto diff = std::chrono::duration_cast<std::chrono::hours>(depTime - selectedTime).count();
                    return std::abs(diff) > 24;
                }),
                flights.end()
            );
        }

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
        std::map<std::string,int> statusOrder = {{"boarding",0}, {"ontime",1}, {"departed",2}};

        if (sort == "departure") {
            std::sort(flights.begin(), flights.end(), [](const FlightItem& a, const FlightItem& b){
                return a.departureTime < b.departureTime;
            });
        } else if (sort == "gate") {
            std::sort(flights.begin(), flights.end(), [](const FlightItem& a, const FlightItem& b){
                return a.gate < b.gate;
            });
        } else {
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

    // GET /api/planes
    CROW_ROUTE(app, "/api/planes").methods(crow::HTTPMethod::GET)
    ([&db]{
        crow::json::wvalue out;
        out["planes"] = db.getAllPlanes();
        return crow::response{200, out};
    });

    // GET /api/airports
    CROW_ROUTE(app, "/api/airports").methods(crow::HTTPMethod::GET)
    ([&db]{
        crow::json::wvalue out;
        out["airports"] = db.getAllAirports();
        return crow::response{200, out};
    });

    // GET /api/airlines
    CROW_ROUTE(app, "/api/airlines").methods(crow::HTTPMethod::GET)
    ([&db]{
        crow::json::wvalue out;
        out["airlines"] = db.getAllAirlines();
        return crow::response{200, out};
    });


    // POST /api/flights
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


    // GET /api/flights/<id>
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

    // PUT /api/flights/<id>
    CROW_ROUTE(app, "/api/flights/<int>").methods(crow::HTTPMethod::PUT)
    ([&db](const crow::request& req, int flightID){
        auto body = crow::json::load(req.body);
        if (!body) return crow::response{400, "Invalid JSON"};

        const char* fields[] = {
            "planeID","originAirportID","destinationAirportID",
            "airline","gate","passengerCount","departureTime"
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
                body["originAirportID"].i(),
                body["destinationAirportID"].i(),
                body["airline"].s(),
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

    // PATCH /api/flights/<id>
    CROW_ROUTE(app, "/api/flights/<int>").methods(crow::HTTPMethod::PATCH)
    ([&db](const crow::request& req, int flightID){
        auto body = crow::json::load(req.body);
        if (!body) return crow::response{400, "Invalid JSON"};

        try {
            // Fetch existing flight (wvalue)
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
            int planeID = body.has("planeID") ? body["planeID"].i() : existing["planeID"].i();
            int originAirportID = body.has("originAirportID") ? body["originAirportID"].i() : existing["originAirportID"].i();
            int destinationAirportID = body.has("destinationAirportID") ? body["destinationAirportID"].i() : existing["destinationAirportID"].i();

            std::string airline = body.has("airline")
                ? std::string(body["airline"].s())
                : std::string(existing["airline"].s());

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

            bool ok = db.updateFlight(
                flightID, planeID, originAirportID, destinationAirportID,
                airline, gate, passengerCount, departureTime
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


    // DELETE /api/flights/<id>
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



    //run
    app.port(18080).multithreaded().run();
    return 0;
}
