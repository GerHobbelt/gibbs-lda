cmake_minimum_required(VERSION 3.15)
project(GibbsLDA)

set(LIBRARY_NAME Gibbslda)

set(CMAKE_CXX_STANDARD 17)
add_definitions(-Wno-unused-result)
include_directories(src)

set(SRCS 
        src/constants.h
        src/dataset.cpp
        src/dataset.h
        src/lda.cpp
        src/model.cpp
        src/model.h
        src/strtokenizer.cpp
        src/strtokenizer.h
        src/utils.cpp
        src/utils.h)

add_executable(gibbslda ${SRCS})

install(TARGETS gibbslda RUNTIME DESTINATION bin)
