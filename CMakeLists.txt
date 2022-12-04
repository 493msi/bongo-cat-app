cmake_minimum_required(VERSION 3.19)
project(bongo_cat_app)

set(CMAKE_CXX_STANDARD 14)

find_package(Threads REQUIRED)

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED SDL2_image::SDL2_image)

add_executable(bongo_cat_app WIN32
        src/BongoCat.cpp)

target_link_libraries(bongo_cat_app SDL2::SDL2 SDL2_image::SDL2_image Threads::Threads)