ADD_LIBRARY(
  puyobot
  solver/eval.c
  solver/policy.c
  solver/ranking.c
  solver/search.c
  template/bottom.c
  template/bottom_match.c
  template/pattern.c
  ui/init.c
  ui/state.c
  animate.c
  bitboard.c
  bottom.c
  deal.c
  multiplayer.c
  record.c
  state.c
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
