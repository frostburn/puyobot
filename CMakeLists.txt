CMAKE_MINIMUM_REQUIRED(VERSION 2.8.7)
PROJECT(puyobot C)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
endif()

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
ADD_EXECUTABLE(merge_clears merge_clears.c)
TARGET_LINK_LIBRARIES(merge_clears puyobot jkiss fulldict)
ADD_EXECUTABLE(demo demo.c)
TARGET_LINK_LIBRARIES(demo puyobot jkiss fulldict m)
ADD_EXECUTABLE(rank_policies rank_policies.c)
TARGET_LINK_LIBRARIES(rank_policies puyobot jkiss fulldict)
ADD_EXECUTABLE(rank_templates rank_templates.c)
TARGET_LINK_LIBRARIES(rank_templates puyobot jkiss fulldict)
ADD_EXECUTABLE(record_policy record_policy.c)
TARGET_LINK_LIBRARIES(record_policy puyobot jkiss fulldict)
ADD_EXECUTABLE(api api/main.c)
TARGET_LINK_LIBRARIES(api puyobot jkiss fulldict)
ADD_EXECUTABLE(multiplayer_demo multiplayer_demo.c)
TARGET_LINK_LIBRARIES(multiplayer_demo puyobot jkiss fulldict m)

FIND_PACKAGE(Curses REQUIRED)
INCLUDE_DIRECTORIES(${CURSES_INCLUDE_DIR})
ADD_EXECUTABLE(ui_endless ui/endless.c)
TARGET_LINK_LIBRARIES(ui_endless ${CURSES_LIBRARIES} puyobot jkiss fulldict)
ADD_EXECUTABLE(ui_versus_bot ui/versus_bot.c)
TARGET_LINK_LIBRARIES(ui_versus_bot ${CURSES_LIBRARIES} puyobot jkiss fulldict)
ADD_EXECUTABLE(ui_playback ui/playback.c)
TARGET_LINK_LIBRARIES(ui_playback ${CURSES_LIBRARIES} puyobot jkiss fulldict)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3")
