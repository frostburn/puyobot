#include <assert.h>
#include <stdio.h>

#include "jkiss/jkiss.h"

#include "puyobot/util.h"

int main() {
    jkiss_init();
    short int values[] = {0, 1, 2, 3, 4, 5, 6, 7};
    shuffle(values, 8, sizeof(short int));
    unsigned char flags = 0;
    for (int i = 0; i < 8; ++i) {
        printf("%d\n", values[i]);
        flags |= 1 << values[i];
    }
    assert(flags = ~0);
}
