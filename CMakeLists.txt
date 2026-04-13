cmake_minimum_required(VERSION 3.16)
project(phone_service VERSION 1.0.0 LANGUAGES CXX)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(BUILD_TESTS "Build tests" OFF)

# Source files
set(SOURCES
    src/main.cpp
    src/server.cpp
    src/tool_registry.cpp
    src/protocol.cpp
    src/tools/search_images.cpp
    src/tools/get_device_info.cpp
    src/tools/send_notification.cpp
    src/tools/get_location.cpp
    src/tools/install_app.cpp
)

# Header files
set(HEADERS
    src/server.h
    src/tool_registry.h
    src/protocol.h
)

# Create executable
add_executable(phone_service ${SOURCES} ${HEADERS})

# nlohmann/json library
set(NLOHMANN_JSON_DIR "/home/jason/projects/ClawAgent/thirdparty/nlohmann")

# Include directories
target_include_directories(phone_service PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools
    ${NLOHMANN_JSON_DIR}/include
)

# Link libraries (none needed for basic socket)

# Install target
install(TARGETS phone_service DESTINATION bin)

# Testing (placeholder for future)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Print configuration summary
message(STATUS "===================================")
message(STATUS "Phone Service Configuration")
message(STATUS "===================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "===================================")
