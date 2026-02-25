cmake_minimum_required(VERSION 3.15)
project(leduc-poker-cfr CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/game.cpp
    src/cfr.cpp
    src/strategy.cpp
    src/agent.cpp
    src/evaluation.cpp
)

# Main training executable
add_executable(cfr_train src/train.cpp ${SOURCES})

# CLI play executable
add_executable(cfr_play src/play.cpp ${SOURCES})

# Tests executable
add_executable(cfr_tests 
    tests/test_game.cpp
    tests/test_cfr.cpp
    tests/test_evaluation.cpp
    ${SOURCES}
)

# Optional: nlohmann/json is header-only and included in include/
# If not available locally, users can install via vcpkg or download the header
