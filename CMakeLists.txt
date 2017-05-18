set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
cmake_minimum_required(VERSION 3.1)
project(GIBBSLDA)

set(LIBRARY_NAME Gibbslda)
find_package(Boost 1.58	COMPONENTS
        thread
        timer
        chrono
        exception
        #regex
        log
        log_setup
        #atomic
        date_time
        serialization
        program_options
        system
        filesystem
        unit_test_framework
        REQUIRED)
include_directories(./include ${Boost_INCLUDE_DIRS})
set(SRCS ./src/ldainfer_gibbs.cpp ./src/dataset.cpp ./src/model.cpp ./src/strtokenizer.cpp ./src/utils.cpp)
add_executable(GIBBSLDA ${SRCS})
target_link_libraries(GIBBSLDA ${Boost_CXX_Driver_LIBRARIES})

#project(GIBBSLDAINFER)
#set(LIBRARY_NAME Gibbsldainfer)
#include_directories(./include)
#set(SRCS1 ./src/ldainfer.cpp ./src/dataset.cpp ./src/model.cpp ./src/strtokenizer.cpp ./src/utils.cpp)
#
#add_executable(GIBBSLDAINFER ${SRCS1})