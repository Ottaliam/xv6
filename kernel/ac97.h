#include "types.h"
#define NUM_SAMPLE  0x0100

struct audionode {
    struct audionode *next;
    uchar data[32][NUM_SAMPLE * 2];
    int flag;
};

void add_sound(struct audionode *node);
void start_play();
void sound_interrupt();
void pause_play();
void continue_play();
void stop_play();
void set_volume(ushort volume);
void set_sample_rate(ushort sample_rate);