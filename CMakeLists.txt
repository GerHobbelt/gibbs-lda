cmake_minimum_required(VERSION 3.15)
project(GibbsLDA)

set(CMAKE_CXX_STANDARD 17)
add_definitions(-Wno-unused-result)
include_directories(src)

add_executable(gibbslda
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

install(TARGETS gibbslda RUNTIME DESTINATION bin)
