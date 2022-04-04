cmake_minimum_required(VERSION 3.1)
project(GIBBSLDA)

set(LIBRARY_NAME Gibbslda)
include_directories(./include)
set(SRCS ./src/lda.cpp ./src/dataset.cpp ./src/model.cpp ./src/strtokenizer.cpp ./src/utils.cpp)

add_executable(GIBBSLDA ${SRCS})
