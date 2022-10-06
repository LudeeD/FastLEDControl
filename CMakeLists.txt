cmake_minimum_required(VERSION 3.10)
project(FastLEDControl VERSION 0.1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(TETON_DEBUG "Build the project using debugging code" OFF)
if(TETON_DEBUG)
  add_definitions(-DTETON_DEBUG)
  SET(CMAKE_BUILD_TYPE Debug)
else()
  set(CMAKE_BUILD_TYPE Release)
  set(CMAKE_CXX_FLAGS_RELEASE "-O2")
endif()
message("-- Building using ${CMAKE_BUILD_TYPE} mode")

option(TETON_BENCHMARK "Build the project using benchmark code" OFF)
if(TETON_BENCHMARK)
  add_definitions(-DTETON_BENCHMARK)
  message("-- Building with benchmarking enabled")
else()
  message("-- Building with benchmarking disabled")
endif()

# Threading library
find_package(Threads REQUIRED)

# OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# MQTT
find_package(PahoMqttCpp REQUIRED)

configure_file(version_config.h.in ${CMAKE_BINARY_DIR}/generated/version_config.h)
include_directories(${CMAKE_BINARY_DIR}/generated/)

# Create an executable
set(MAIN_SOURCES
  main.cpp
  src/utils/utils.cpp
  src/network/client.cpp
  src/network/circular_buffer.cpp
)

set(MAIN_LIBRARIES
  ${CUDA_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${OpenCV_LIBS}
  boost_system
  boost_thread
  boost_date_time
  crypto
  cpprest
  paho-mqttpp3
  paho-mqtt3as
)

add_executable(${PROJECT_NAME} ${MAIN_SOURCES})
target_link_libraries(${PROJECT_NAME} ${MAIN_LIBRARIES})
target_include_directories(${PROJECT_NAME} PUBLIC include/)
