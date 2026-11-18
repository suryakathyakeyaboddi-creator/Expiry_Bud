cmake_minimum_required(VERSION 3.16)
project(ExpiryBud VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EXPIRYBUD_SOURCES
    src/Text.cpp
    src/Date.cpp
    src/Models.cpp
    src/Storage.cpp
    src/Services.cpp
    src/Application.cpp
)

add_library(expirybud_lib ${EXPIRYBUD_SOURCES})
target_include_directories(expirybud_lib PUBLIC include)
target_compile_options(expirybud_lib PRIVATE -Wall -Wextra -Wpedantic)

add_executable(expirybud src/main.cpp)
target_link_libraries(expirybud PRIVATE expirybud_lib)

enable_testing()
add_executable(expirybud_tests tests/test_main.cpp)
target_link_libraries(expirybud_tests PRIVATE expirybud_lib)
add_test(NAME expirybud_tests COMMAND expirybud_tests)
