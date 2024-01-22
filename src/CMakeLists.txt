add_executable(${PROJECT_NAME} Emulator.cpp
        components/Memory.cpp
        components/Memory.h)
target_link_libraries(${PROJECT_NAME}
        PRIVATE z80
        PRIVATE SDL2::SDL2main
        PRIVATE SDL2::SDL2-static)
target_include_directories(${PROJECT_NAME}
        PRIVATE ${SDL2_INCLUDE_DIR})