cmake_minimum_required(VERSION 3.10)
project(zen VERSION 1.0.0 LANGUAGES C)

# C Standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS_DEBUG "-g -Wall -Wextra -Werror -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Source files
file(GLOB CORE_SOURCES src/core/*.c)
file(GLOB TYPE_SOURCES src/types/*.c)
file(GLOB RUNTIME_SOURCES src/runtime/*.c)
file(GLOB STDLIB_SOURCES src/stdlib/*.c)

# Currently only core sources exist
set(ZEN_SOURCES 
    src/main.c
    src/lexer.c
    src/parser.c
    src/AST.c
    src/visitor.c
    src/scope.c
    src/token.c
    src/io.c
)

# Include directories
include_directories(src/include)

# Find external dependencies
find_package(PkgConfig REQUIRED)

# cJSON
pkg_check_modules(CJSON REQUIRED libcjson)

# YAML (for future use)
# pkg_check_modules(YAML REQUIRED yaml-0.1)

# PCRE2 (for future use)
# pkg_check_modules(PCRE2 REQUIRED libpcre2-8)

# CURL (for future use)
# find_package(CURL REQUIRED)

# SQLite3 (optional, for future use)
# find_package(SQLite3)

# Main executable
add_executable(zen ${ZEN_SOURCES})

# Include directories
target_include_directories(zen PRIVATE ${CJSON_INCLUDE_DIRS})

# Link libraries
target_link_libraries(zen ${CJSON_LIBRARIES})
# Future: target_link_libraries(zen ${CJSON_LIBRARIES} ${YAML_LIBRARIES} ${PCRE2_LIBRARIES} ${CURL_LIBRARIES})

# Tools (future implementation)
# add_executable(zenfmt tools/zenfmt/main.c tools/zenfmt/formatter.c)
# add_executable(zenlint tools/zenlint/main.c tools/zenlint/rules.c)

# Testing
enable_testing()

# Unit tests (future implementation)
# find_package(CMocka REQUIRED)
# file(GLOB TEST_SOURCES tests/unit/*.c tests/framework/*.c)
# add_executable(zen_tests ${TEST_SOURCES})
# target_link_libraries(zen_tests cmocka)
# add_test(NAME unit_tests COMMAND zen_tests)

# Installation
install(TARGETS zen DESTINATION bin)
install(FILES README.md LICENSE DESTINATION share/zen)
install(DIRECTORY examples/ DESTINATION share/zen/examples)
install(DIRECTORY docs/ DESTINATION share/zen/docs)

# CPack configuration
set(CPACK_PACKAGE_NAME "zen")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ZEN - A lightweight, mobile-friendly scripting language")
set(CPACK_PACKAGE_VENDOR "akaoio")
set(CPACK_PACKAGE_CONTACT "Nguyen Ky Son")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

include(CPack)