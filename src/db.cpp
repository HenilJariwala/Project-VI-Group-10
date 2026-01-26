#include "db.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

Db::Db(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
}

Db::~Db() {
    if (db_) sqlite3_close(db_);
}

void Db::initSchema(const std::string& schemaPath) {
    std::ifstream file(schemaPath);
    if (!file) {
        throw std::runtime_error("Could not open schema.sql");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    char* err = nullptr;
    if (sqlite3_exec(db_, buffer.str().c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "Unknown SQL error";
        sqlite3_free(err);
        throw std::runtime_error(msg);
    }
}
