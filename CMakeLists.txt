cmake_minimum_required(VERSION 3.16)
project(robomaster_assessment LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(assessment_contracts INTERFACE)
target_include_directories(assessment_contracts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(contract_compile_test tests/contract_compile_test.cpp)
target_link_libraries(contract_compile_test PRIVATE assessment_contracts)

get_property(_rm_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_rm_multi_config)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  foreach(_config IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_config} ${CMAKE_BINARY_DIR})
  endforeach()
endif()

if(MINGW)
  add_link_options(-static -static-libgcc -static-libstdc++)
endif()

add_executable(f4_cases tests/f4_cases.cpp)
target_link_libraries(f4_cases PRIVATE assessment_contracts)
enable_testing()
add_test(NAME f4_cases COMMAND f4_cases
  ${CMAKE_CURRENT_SOURCE_DIR}/data/F4/F4_ballistic_cases.jsonl)

add_executable(f1_cases tests/f1_cases.cpp)
target_link_libraries(f1_cases PRIVATE assessment_contracts)
enable_testing()
add_test(NAME f1_cases COMMAND f1_cases
  ${CMAKE_CURRENT_SOURCE_DIR}/data/F1/F1_lightbar_candidates.csv)