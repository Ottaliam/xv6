#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if(argc != 1){
        fprintf(2, "Usage: pingpong\n");
        exit(1);
    }

    char buf[1];
    int p[2];
    pipe(p);

    if(fork() == 0){
        read(p[0], buf, 1);
        printf("%d: received ping\n", getpid());
        write(p[1], "b", 1);
        exit(0);
    } else{
        write(p[1], "a", 1);
        read(p[0], buf, 1);
        printf("%d: received ping\n", getpid());
    }

    close(p[0]);
    close(p[1]);

    exit(0);
}