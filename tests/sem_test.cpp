#include "catalog/catalog.h"
#include "storage/table.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstring>

namespace fs = std::filesystem;

int main() {

    // -----------------------------------------
    // 1. Remove old test database
    // -----------------------------------------

    fs::remove("data/SemanticTest/data.db");


    // -----------------------------------------
    // 2. Create table metadata
    // -----------------------------------------

    TableMeta meta{};

    std::strncpy(
        meta.name,
        "SemanticTest",
        tns - 1
    );

    meta.name[tns - 1] = '\0';


    // PK is STRING because vector PKs are:
    // mb-0, mb-1, ...

    meta.columns.push_back(
        ColMeta(
            "id",
            STRING,
            true,
            16
        )
    );


    // Just some payload so the table has
    // a normal non-PK column as well.

    meta.columns.push_back(
        ColMeta(
            "text",
            STRING,
            false,
            64
        )
    );


    // -----------------------------------------
    // 3. Create table using MemoraDB
    // -----------------------------------------

    {
        Catalog catalog;

        if(!catalog.createTable(meta)) {
            std::cerr << "Failed to create SemanticTest table.\n";
            return 1;
        }
    }


    // -----------------------------------------
    // 4. Append controlled records
    // -----------------------------------------

    std::string path =
        "data/SemanticTest/data.db";

    std::fstream file(
        path,
        std::ios::binary |
        std::ios::in |
        std::ios::out
    );

    if(!file) {
        std::cerr << "Failed to open data.db.\n";
        return 1;
    }


    file.seekp(0, std::ios::end);


    const std::string pks[] = {
        "mb-0",
        "mb-1",
        "mb-2",
        "mb-3",
        "mb-4"
    };


    const uint64_t timestamps[] = {
        200000,
        200001,
        200002,
        200003,
        200004
    };


    for(int i = 0; i < 5; i++) {

        // Record header
        uint64_t timestamp = timestamps[i];
        bool deleted = false;

        file.write(
            reinterpret_cast<const char*>(&timestamp),
            sizeof(timestamp)
        );

        file.write(
            reinterpret_cast<const char*>(&deleted),
            sizeof(deleted)
        );


        // id column: 16 bytes
        char id[16] = {};

        std::memcpy(
            id,
            pks[i].data(),
            pks[i].size()
        );

        file.write(id, 16);


        // text column: 64 bytes
        char text[64] = {};

        std::string value =
            "semantic test document " +
            std::to_string(i);

        std::memcpy(
            text,
            value.data(),
            value.size()
        );

        file.write(text, 64);
    }


    file.close();

    std::cout
        << "SemanticTest database created successfully.\n";

    return 0;
}