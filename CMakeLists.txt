cmake_minimum_required(VERSION 3.15)
project(GibbsLDA)

set(LIBRARY_NAME Gibbslda)

set(CMAKE_CXX_STANDARD 17)
#add_definitions(-Wno-unused-result)
include_directories(include)

set(SRCS 
        include/constants.h
        src/dataset.cpp
        include/dataset.h
        src/lda.cpp
        src/model.cpp
        include/model.h
        src/strtokenizer.cpp
        include/strtokenizer.h
        src/utils.cpp
        include/utils.h)

add_executable(gibbslda ${SRCS})

install(TARGETS gibbslda RUNTIME DESTINATION bin)
