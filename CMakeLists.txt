cmake_minimum_required(VERSION 3.15)

project(Chess)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

add_executable(chess
    src/main.cpp
    src/game.cpp
    src/logic.cpp
    src/gui.cpp
    src/game.h
    src/logic.h
    src/gui.h
    src/definitions.h
    src/engine.cpp
    src/engine.h
)

target_include_directories(chess PRIVATE src)

add_custom_command(TARGET chess POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets       
    $<TARGET_FILE_DIR:chess>/assets 
)

target_link_libraries(chess PRIVATE SFML::Graphics SFML::Window SFML::System)