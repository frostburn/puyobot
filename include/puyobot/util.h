#ifndef PUYOBOT_UTIL_H_GUARD
#define PUYOBOT_UTIL_H_GUARD

#include <stddef.h>

char* file_to_buffer(char *filename);

// Arrange the N elements of ARRAY in random order.
void shuffle(void *array, size_t n, size_t element_size);

#endif
