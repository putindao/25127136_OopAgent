// =============================================================================
//  main.cpp — application entry point.
//  P0: a toolchain check that proves libcurl + nlohmann/json + sqlite3 link and
//  run. Later phases replace this body with the real agent CLI driver.
// =============================================================================
#include <cstdio>

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sqlite3.h>

int main() {
    std::puts("OOP AI Agent — toolchain check");

    // libcurl: HTTP client backing OllamaClient.
    std::printf("  libcurl       : %s\n", curl_version());

    // nlohmann/json: build a tiny object to confirm the header works.
    const nlohmann::json probe = {{"model", "gemma4"}, {"ok", true}};
    std::printf("  nlohmann/json : %d.%d.%d (probe=%s)\n",
                NLOHMANN_JSON_VERSION_MAJOR, NLOHMANN_JSON_VERSION_MINOR,
                NLOHMANN_JSON_VERSION_PATCH, probe.dump().c_str());

    // sqlite3: open an in-memory database and close it.
    sqlite3* db = nullptr;
    const int rc = sqlite3_open(":memory:", &db);
    std::printf("  sqlite3       : %s (open rc=%d)\n", sqlite3_libversion(), rc);
    sqlite3_close(db);

    std::puts("All three libraries linked OK.");
    return 0;
}
