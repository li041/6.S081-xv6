#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int pid;
    int p2c[2];
    int c2p[2];
    char buf[16];
    pipe(p2c);
    pipe(c2p);
    if((pid = fork()) == 0){    /* Child */
        close(p2c[1]);
        close(c2p[0]);

        read(p2c[0], buf, sizeof(char));
        close(p2c[0]);
        printf("%d: received ping\n", getpid());
        write(c2p[1], buf, sizeof(char));
        close(c2p[1]);
        exit(0);
    }

    close(p2c[0]);
    close(c2p[1]);

    write(p2c[1], buf, sizeof(char));
    close(p2c[1]);
    read(c2p[0], buf, sizeof(char));
    close(c2p[0]);
    printf("%d: received pong\n", getpid());
    exit(0);
}