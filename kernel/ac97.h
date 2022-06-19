#include "types.h"
#define NUM_SAMPLE  0x0100

struct audionode {
    struct audionode *next;
    uchar data[32][NUM_SAMPLE * 2];
    int used;
};

void BDL_add_entry(struct audionode *node);
void start_play();
void pause_play();
void continue_play();
void stop_play();
void set_volume(ushort volume);