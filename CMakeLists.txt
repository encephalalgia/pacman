cmake_minimum_required(VERSION 3.26)
project(pacman
        LANGUAGES CXX
        VERSION 0.0.0
        DESCRIPTION "A Pac-Man arcade emulator"
        HOMEPAGE_URL "https://github.com/encephalalgia/pacman"
)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
        z80
        GIT_REPOSITORY "https://github.com/encephalalgia/z80.git"
)
set(TEST_Z80 OFF)
FetchContent_MakeAvailable(z80)

find_package(SDL2 REQUIRED)
add_subdirectory(src)
