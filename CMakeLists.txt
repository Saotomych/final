cmake_minimum_required(VERSION 2.8) # Проверка версии CMake.
project(final)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#add_definitions("-std=c++11")
add_executable(final httpserver.cpp)		# Создает исполняемый файл с именем main
target_link_libraries(final pthread)

