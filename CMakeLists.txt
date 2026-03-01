cmake_minimum_required(VERSION 3.15)
project(leduc-cfr CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi")
endif()

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/game_state.cpp
    src/infoset.cpp
    src/cfr_solver.cpp
    src/exploitability.cpp
    src/strategy.cpp
)

# Main executable
add_executable(leduc-cfr src/main.cpp ${SOURCES})

# Test executable
add_executable(leduc-tests
    tests/test_main.cpp
    tests/test_game_state.cpp
    tests/test_infoset.cpp
    tests/test_cfr.cpp
    tests/test_exploitability.cpp
    ${SOURCES}
)

# Benchmark executable (optional)
add_executable(leduc-bench
    benchmarks/bench_cfr.cpp
    benchmarks/bench_game.cpp
    ${SOURCES}
)
