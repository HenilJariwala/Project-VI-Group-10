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

    crow::SimpleApp app;

    // flights is the main page
    CROW_ROUTE(app, "/")([]{
        crow::response res;
        res.code = 302;
        res.set_header("Location", "/pages/flights.html");
        return res;
    });

    // pages routes
    CROW_ROUTE(app, "/pages/<string>")([](const std::string& file){
        return serveFile("/app/public/pages/" + file, "text/html; charset=utf-8");
    });

    // styles routes
    CROW_ROUTE(app, "/styles/<string>")([](const std::string& file){
        return serveFile("/app/public/styles/" + file, "text/css; charset=utf-8");
    });

    //run
    app.port(18080).multithreaded().run();
    return 0;
}
