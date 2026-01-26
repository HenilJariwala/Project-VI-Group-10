#include "crow_all.h"

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([] {
        return "<h1>Hello Project 6</h1>";
    });

    app.port(18080).multithreaded().run();
    return 0;
}
