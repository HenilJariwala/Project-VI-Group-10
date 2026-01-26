#pragma once
#include <sqlite3.h>
#include <string>

class Db {
public:
    explicit Db(const std::string& path);
    ~Db();

    void initSchema(const std::string& schemaPath);

private:
    sqlite3* db_{nullptr};
};
