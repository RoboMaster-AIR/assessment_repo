cmake_minimum_required(VERSION 3.16)
project(robomaster_assessment LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(assessment_contracts INTERFACE)
target_include_directories(assessment_contracts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(contract_compile_test tests/contract_compile_test.cpp)
target_link_libraries(contract_compile_test PRIVATE assessment_contracts)
