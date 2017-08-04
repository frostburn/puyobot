ADD_LIBRARY(
  puyobot
  bitboard.c
  bottom.c
  deal.c
  tablebase.c
  util.c
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
