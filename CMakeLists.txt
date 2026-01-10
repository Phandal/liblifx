cmake_minimum_required(VERSION 3.16..4.0)

project(liblifx VERSION 0.1.0
  DESCRIPTION "Library to interact with Lifx Lan API"
  LANGUAGES C)

add_library(lifx STATIC lib/frame.c)
target_include_directories(lifx PUBLIC "include")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(demo src/main.c)
target_link_libraries(demo lifx)
target_include_directories(demo PRIVATE "include")

add_executable(discover src/discover.c)
target_link_libraries(discover lifx)
target_include_directories(discover PRIVATE "include")

add_executable(sdl src/sdl.c)
target_link_libraries(sdl lifx)
target_include_directories(sdl PRIVATE "include")
