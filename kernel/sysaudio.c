#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "ac97.h"
#include "proc.h"

static int play_pid = 0;
char buffer[32 * NUM_SAMPLE * 2];
struct audionode nodes[3];

uint64 sys_writeplaypid(void)
{
    int pid;
    if(argint(0, &pid) < 0)
        return -1;

    play_pid = pid;
    return 0;
}

uint64 sys_readplaypid(void)
{
    return play_pid;
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

uint64 sys_setsamplerate(void)
{
    int val;
    if(argint(0, &val) < 0)
        return -1;
    
    // Going to play a new sound, clear buffer
    memset(nodes, 0, sizeof(nodes));

    set_sample_rate(val);
    return 0;
}

uint64 sys_writeaudio(void)
{
    uint64 buf_addr;
    int size;

    if(argaddr(0, &buf_addr) < 0)
        return -1;
    if(argint(1, &size) < 0)
        return -1;
    
    struct proc *p = myproc();
    if(copyin(p->pagetable, buffer, buf_addr, size) < 0)
        return -1;

    int suc = 1;
    while(suc)
    {
        for(int i = 0; i < 3; ++i)
            if(nodes[i].flag == 0)
            {
                memset(nodes[i].data, 0, 32 * NUM_SAMPLE * 2);
                memmove(nodes[i].data, buffer, size);
                nodes[i].flag = 1;
                nodes[i].next = 0;

                add_sound(&nodes[i]);

                suc = 0;
                break;
            }
    }
    return 0;
}