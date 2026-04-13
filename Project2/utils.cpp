#include "utils.h"
#include <stdlib.h>
#include <time.h>

void InitRandom() {
    srand((unsigned int)time(NULL));
}

int GetRandomInt(int min, int max) {
    if (max < min) return min;
    return min + rand() % (max - min + 1);
}
