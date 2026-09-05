cmake_minimum_required(VERSION 3.16)
project(orderbook_engine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to a build with optimizations and debug info if the user
# didn't pick a build type.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# --- The library: all the actual engine code ---
add_library(orderbook
  src/order.cpp
)

target_include_directories(orderbook PUBLIC include)

target_compile_options(orderbook PRIVATE
  -Wall -Wextra -Wpedantic
)

# --- The executable that uses the library ---
add_executable(engine apps/main.cpp)
target_link_libraries(engine PRIVATE orderbook)