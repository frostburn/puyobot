#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "jkiss/jkiss.h"

#include "puyobot/util.h"

char* file_to_buffer(char *filename) {
    FILE * pFile;
    size_t lSize;
    char * buffer;
    size_t result;

    pFile = fopen(filename , "rb");
    if (pFile == NULL) {
        fprintf(stderr, "File error\n");
        exit (1);
    }

    // obtain file size:
    fseek(pFile , 0 , SEEK_END);
    lSize = ftell(pFile);
    rewind(pFile);

    // allocate memory to contain the whole file:
    buffer = (char*) malloc(sizeof(char) * lSize);
    if (buffer == NULL) {
        fprintf(stderr, "Memory error");
        exit (2);
    }

    // copy the file into the buffer:
    result = fread(buffer, 1, lSize, pFile);
    if (result != lSize) {
        fprintf(stderr, "Reading error");
        exit (3);
    }

    /* the whole file is now loaded in the memory buffer. */

    // terminate
    fclose(pFile);
    return buffer;
}

// Arrange the N elements of ARRAY in random order.
void shuffle(void *array, size_t n, size_t element_size) {
    if (n <= 1) {
        return;
    }
    void *a1;
    void *a2;
    void *temp = malloc(element_size);
    for (size_t i = 0; i < n - 1; ++i) {
      size_t j = i + jrand() % (n - i);
      a1 = array + i * element_size;
      a2 = array + j * element_size;
      memcpy(temp, a2, element_size);
      memcpy(a2, a1, element_size);
      memcpy(a1, temp, element_size);
    }
    free(temp);
}

int bitset_rand_index(unsigned int bitset) {
    if (!bitset) {
        return -1;
    }
    int n = jrand() % __builtin_popcount(bitset);
    int i;
    for (i = 0;; ++i) {
        if (bitset & (1 << i)) {
            if (n-- == 0) {
                return i;
            }
        }
    }
    return i;
}
