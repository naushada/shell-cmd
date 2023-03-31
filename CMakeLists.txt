cmake_minimum_required(VERSION 3.16.3)
project(shell-service)

add_subdirectory(test)


include_directories(inc)
include_directories(/usr/local/ACE_TAO-7.0.0/include)

link_directories(/usr/local/ACE_TAO-7.0.0/lib)

#Adding Complier flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -g -Wall -Wextra")

#However, the file(GLOB...) allows for wildcard additions:
file(GLOB SOURCES "src/*.cc")

add_executable(shellsvc ${SOURCES})
target_link_libraries(shellsvc pthread)
target_link_libraries(shellsvc ACE)
target_link_libraries(shellsvc ACE_SSL)
target_link_libraries(shellsvc ssl crypto)

