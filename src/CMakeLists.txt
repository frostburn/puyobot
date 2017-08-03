ADD_LIBRARY(
  puyobot
  bitboard.c
  bottom.c
)
INSTALL(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/puyobot
  DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
INSTALL(
  TARGETS puyobot
  ARCHIVE
  DESTINATION lib
)
