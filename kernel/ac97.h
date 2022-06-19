#include "types.h"
#define NUM_SAMPLE  0x0100

struct audionode {
    struct audionode *next;
    uchar data[32][NUM_SAMPLE * 2];
    int used;
};