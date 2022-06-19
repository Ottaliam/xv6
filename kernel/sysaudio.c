#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "ac97.h"

uint64 sys_addaudio(void)
{
    uint64 p;
    if(argaddr(0, &p) < 0)
        return -1;
    BDL_add_entry((struct audionode*)p);
    return 0;
}

uint64 sys_startplay(void)
{
    start_play();
    return 0;
}

uint64 sys_pauseplay(void)
{
    pause_play();
    return 0;
}

uint64 sys_continueplay(void)
{
    continue_play();
    return 0;
}

uint64 sys_stopplay(void)
{
    stop_play();
    return 0;
}

uint64 sys_setvolume(void)
{
    int val;
    if(argint(0, &val) < 0)
        return -1;
    
    if(val < 0 || val > 100)
        return -1;

    val = (100.0 - val) / 100.0 * 63.0;
    set_volume(((val & 0x3F) << 8) | (val & 0x3F));

    return 0;
}