#pragma once
#include <cinttypes>
#include <string>



constexpr bool BROADCAST_ERRORS = 1;

#define LOG_ASSERT(predicate, log) assert(predicate); if (!predicate and BROADCAST_ERRORS) { std::cout << log << '\n'; } else {}

#define ERROR(log, erc) \
    if constexpr (BROADCAST_ERRORS) { \
        std::println("err; log: {}, erc: {}", (log), (erc)); \
    } else {}\

#define BOOST_BEAST_FAIL(ec, what) \
    if constexpr (BROADCAST_ERRORS) { \
        std::cerr << what << ": " << ec.message() << "\n"; \
    } else{}\

constexpr int32_t CLIENT_DISCONNECTED_UNKNOWN = -1;
constexpr int32_t CLIENT_DISCONNECTED_NORMALLY = 0;
constexpr int32_t CLIENT_DISCONNECTED_UNEXPECTED = 1;
constexpr int32_t CLIENT_DISCONNECTED_SHUT_DOWN_BY_SERVER = 2;

constexpr int32_t FILE_ERROR_UNKNOWN = -1;
constexpr int32_t FILE_ERROR_NONE = 0;
constexpr int32_t FILE_ERROR_NOT_FOUND = 1;
constexpr int32_t FILE_ERROR_COULD_NOT_BE_READ = 2;
constexpr int32_t FILE_ERROR_INVALID_ACCESS = 3;

const std::string PROJECT_ROOT = ".";
const std::string CLIENT_ROOT = PROJECT_ROOT + "/client";
const std::string SERVER_ROOT = PROJECT_ROOT + "/server";
const std::string BUILD_ROOT = SERVER_ROOT + "/build";
const std::string INDEX_HTML_PATH = CLIENT_ROOT + "/index.html";
const std::string DIST_ROOT = CLIENT_ROOT + "/dist";
const std::string MAIN_JS_PATH = DIST_ROOT + "/main/main.js";
