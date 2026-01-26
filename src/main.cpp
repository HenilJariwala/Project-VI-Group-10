#include "crow_all.h"
#include "db.h"
#include <filesystem>
#include <iostream>

int main() {
    std::filesystem::create_directories("/app/runtime_db");

    Db db("/app/runtime_db/flights.db");
    db.initSchema("/app/src/schema.sql");

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([] {
        return "<h1>Hello Project 6</h1><p>made database.</p>";
    });

    app.port(18080).multithreaded().run();
    return 0;
}
