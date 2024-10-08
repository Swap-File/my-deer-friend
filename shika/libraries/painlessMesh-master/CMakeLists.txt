cmake_minimum_required (VERSION 3.5)
project (painlessMesh)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

FIND_PACKAGE(Boost CONFIG REQUIRED COMPONENTS system)
IF(Boost_FOUND)
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF(Boost_FOUND)

FILE(GLOB TESTFILES test/**/catch_*.cpp)
foreach(TESTFILE ${TESTFILES})
    get_filename_component(NAME ${TESTFILE} NAME_WE) 
    add_executable(${NAME} ${TESTFILE} test/catch/fake_serial.cpp src/scheduler.cpp)
    target_include_directories(${NAME} PUBLIC test/include/ test/catch/ test/ArduinoJson/src/ src/ test/TaskScheduler/src)
endforeach()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(catch_tcp_integration test/boost/tcp_integration.cpp test/catch/fake_serial.cpp src/scheduler.cpp)
target_include_directories(catch_tcp_integration PUBLIC test/include/ test/boost/ test/ArduinoJson/src/ test/TaskScheduler/src/ src/)
TARGET_LINK_LIBRARIES(catch_tcp_integration ${Boost_LIBRARIES})

add_executable(catch_connection test/boost/connection.cpp test/catch/fake_serial.cpp src/scheduler.cpp)
target_include_directories(catch_connection PUBLIC test/include/ test/boost/ test/ArduinoJson/src/ test/TaskScheduler/src/ src/)
TARGET_LINK_LIBRARIES(catch_connection ${Boost_LIBRARIES})
