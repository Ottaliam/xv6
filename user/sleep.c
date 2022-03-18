#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if(argc != 2){
        fprintf(2, "Usage: sleep time\n");
        exit(1);
    }

    int t = atoi(argv[1]);
    if(sleep(t) < 0){
        fprintf(2, "sleep: failed\n");
        exit(1);
    }

    exit(0);
}