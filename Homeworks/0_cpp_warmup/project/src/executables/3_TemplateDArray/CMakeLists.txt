cmake_minimum_required(VERSION 3.14)
project(3_TemplateDArray LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(3_TemplateDArray main.cpp)

set_target_properties(3_TemplateDArray PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
