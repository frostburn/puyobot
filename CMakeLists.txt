CMAKE_MINIMUM_REQUIRED(VERSION 2.8.7)
PROJECT(puyobot C)

SET(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
SET(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")

INCLUDE_DIRECTORIES(
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/full-dict/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/jkiss/include"
)
LINK_DIRECTORIES(${LIBRARY_OUTPUT_PATH})

ADD_SUBDIRECTORY(full-dict/src)
ADD_SUBDIRECTORY(jkiss/src)
ADD_SUBDIRECTORY(src)

ENABLE_TESTING()
ADD_SUBDIRECTORY(test)

ADD_EXECUTABLE(unique_deals unique_deals.c)
TARGET_LINK_LIBRARIES(unique_deals puyobot jkiss fulldict)
ADD_EXECUTABLE(tabulate_openings tabulate_openings.c)
TARGET_LINK_LIBRARIES(tabulate_openings puyobot jkiss fulldict)
ADD_EXECUTABLE(tabulate_clear_chance tabulate_clear_chance.c)
TARGET_LINK_LIBRARIES(tabulate_clear_chance puyobot jkiss fulldict)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3")

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
endif()
