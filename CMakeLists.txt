cmake_minimum_required(VERSION 3.15)
project(gibbs_lda)

set(CMAKE_CXX_STANDARD 14)

include_directories(src)

add_executable(gibbs_lda
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
