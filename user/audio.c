#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/ac97.h"
#include "user/user.h"

struct fmt {
    uint id;
    uint size;
    ushort audio_format;
    ushort num_channel;
    uint sample_rate;
    uint bytes_rate;
    ushort block_align;
    ushort bits_per_sample;
};

struct wav
{
    uint riff_id;
    uint riff_size;
    uint riff_type;
    struct fmt info;
    uint data_id;
    uint data_size;
};

char buffer[32 * NUM_SAMPLE * 2];

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        fprintf(2, "Usage: audio play/pause/stop/volume\n");
        exit(1);
    }
    else if(strcmp(argv[1], "play") == 0)
    {
        if(argc != 3)
        {
            fprintf(2, "Usage: audio play xxx\n");
            exit(1);
        }

        int pid = fork(), play_pid = readplaypid();
        if(pid > 0)
            exit(0);    // Father Exit
        
        pid = getpid();

        if(play_pid && play_pid != pid) // Now Playing, Stop it
        {
            kill(play_pid);
            stopplay();
        }
        writeplaypid(pid);


        int fd = open(argv[2], O_RDONLY);
        if(fd < 0)
        {
            fprintf(2, "audio play: cannot open %s\n", argv[2]);
            exit(1);
        }

        // Check if it is wav
        struct wav wavhead;
        read(fd, &wavhead, sizeof(struct wav));
        if(wavhead.riff_id != 0x46464952 || wavhead.riff_type != 0x45564157)
        {
            fprintf(2, "audio play: file not in wav format\n");
            close(fd);
            exit(1);
        }

        // Check file format and Set sample rate
        printf("Channel: %d\n", wavhead.info.num_channel);
        printf("Bit per sample: %d\n", wavhead.info.bits_per_sample);
        if(wavhead.info.num_channel != 0x2 || wavhead.info.bits_per_sample != 0x10)
        {
            fprintf(2, "audio play: wav file format not supported\n");
            close(fd);
            exit(1);
        }
        setsamplerate((int)(wavhead.info.sample_rate));

        // Read and Play
        int read_bytes_num;
        while(1)
        {
            read_bytes_num = read(fd, buffer, 32 * NUM_SAMPLE * 2);
            writeaudio(buffer, read_bytes_num);
            if(read_bytes_num < 32 * NUM_SAMPLE * 2)
                break;
        }
        close(fd);

        writeplaypid(0);
        exit(0);
    }
    else if(strcmp(argv[1], "pause") == 0)
    {
        if(argc != 2)
        {
            fprintf(2, "Usage: audio pause\n");
            exit(1);
        }

        pauseplay();
    }
    else if(strcmp(argv[1], "continue") == 0)
    {
        if(argc != 2)
        {
            fprintf(2, "Usage: audio continue\n");
            exit(1);
        }

        continueplay();
    }
    else if(strcmp(argv[1], "stop") == 0)
    {
        if(argc != 2)
        {
            fprintf(2, "Usage: audio stop\n");
            exit(1);
        }

        stopplay();
    }
    else if(strcmp(argv[1], "volume") == 0)
    {
        if(argc != 3)
        {
            fprintf(2, "Usage: audio volume xxx(0-100)\n");
            exit(1);
        }

        int val = atoi(argv[2]);
        if(val < 0 || val > 63)
        {
            fprintf(2, "Usage: audio volume xxx(0-100)\n");
            exit(1);
        }

        setvolume(val);
    }
    else
    {
        fprintf(2, "Usage: audio play/pause/continue/stop/volume\n");
        exit(1);
    }

    exit(0);
}