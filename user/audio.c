#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/ac97.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    if(strcmp(argv[1], "play") == 0)
    {
        //todo: do play
    }
    else if(strcmp(argv[1], "pause") == 0)
    {
        if(argc != 2)
        {
            fprintf(2, "Usage: audio pause\n");
            exit(1);
        }

        pause_play();
    }
    else if(strcmp(argv[1], "stop") == 0)
    {
        if(argc != 2)
        {
            fprintf(2, "Usage: audio stop\n");
            exit(1);
        }

        stop_play();
    }
    else if(strcmp(argv[1], "volume") == 0)
    {
        if(argc != 3)
        {
            fprintf(2, "Usage: audio volume xxx(0-100)\n");
            exit(1);
        }

        int val = atoi(argv[2]);
        if(val < 0 || val > 100)
        {
            fprintf(2, "Usage: audio volume xxx(0-100)\n");
            exit(1);
        }

        val = (100.0 - val) / 100.0 * 63.0;
        set_volume(((val & 0x3F) << 8) | (val & 0x3F));
    }
    else
    {
        fprintf(2, "Usage: audio play/pause/stop/volume\n");
        exit(1);
    }

    exit(0);
}