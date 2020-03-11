cmake_minimum_required(VERSION 3.3.2)
project(lda C CXX)
set (CMAKE_CXX_STANDARD 11)

SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "/home/louisspace/boost")
SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "/home/louisspace/boost/lib")
set(BUILD_SHARED_LIBS ON)
#list(APPEND CMAKE_PREFIX_PATH "/home/louisspace/boost/")
find_package(Boost 1.70 REQUIRED 
             COMPONENTS 
             #program_options 
             #unit_test_framework
             serialization )

link_directories(${Boost_LIBRARY_DIRS})
MESSAGE(STATUS ${BOOST_ROOT})
MESSAGE(STATUS ${Boost_INCLUDE_DIRS})
MESSAGE(STATUS ${Boost_LIBRARIES})
MESSAGE(STATUS ${Boost_LIBRARY_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
include_directories("./src")
add_subdirectory(./src)
#add_subdirectory(./test_serialize)

