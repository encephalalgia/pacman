cmake_minimum_required(VERSION 3.26)
project(pacman)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
        z80
        GIT_REPOSITORY "https://github.com/harrow22/z80.git"
)
set(TEST_Z80 OFF)
FetchContent_MakeAvailable(z80)

find_package(SDL2 REQUIRED)
add_subdirectory(src)
