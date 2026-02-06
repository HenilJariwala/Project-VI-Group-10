#include "crow_all.h"
#include "db.h"
#include <filesystem>
#include <fstream>
#include <sstream>

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
    ([&db]{
        crow::json::wvalue out;
        out["flights"] = db.getAllFlights();
        return crow::response{200, out};
    });

    // assets routes (images, gifs)
    CROW_ROUTE(app, "/assets/<string>")([](const std::string& file){
        return serveFile("/app/public/assets/" + file, "image/gif");
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

    // POST /api/flights
    CROW_ROUTE(app, "/api/flights").methods(crow::HTTPMethod::POST)
    ([&db](const crow::request& req){
        auto body = crow::json::load(req.body);
        if (!body) return crow::response{400, "Invalid JSON"};

        // Required fields
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
            int id = db.createFlight(
                body["planeID"].i(),
                body["originAirportID"].i(),
                body["destinationAirportID"].i(),
                std::string(body["airline"].s()),
                std::string(body["gate"].s()),
                body["passengerCount"].i(),
                std::string(body["departureTime"].s())
                
            );

            crow::json::wvalue out;
            out["message"] = "Flight created";
            out["flightID"] = id;
            return crow::response{201, out};
        } catch (const std::exception& e) {
            return crow::response{500, e.what()};
        }
    });

    //run
    app.port(18080).multithreaded().run();
    return 0;
}
