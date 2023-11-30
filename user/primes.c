#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void sieve(int *pipefl){
    int num, prim;
    int pipetr[2];
    close(pipefl[1]); /* close the write end of pipe from left neighbor */


    if(read(pipefl[0], &prim, sizeof(int)) != 0){ /* read from left neighbor */
        printf("prime %d\n", prim);

        pipe(pipetr);   /* create pipe to right neighbor */
        if(fork() == 0){    /* Create right neighbor */
            sieve(pipetr);
            wait(0);
            exit(0);
        }

        close(pipetr[0]);
        while(read(pipefl[0], &num, sizeof(int)) != 0){
            if(num % prim != 0){
                write(pipetr[1], &num, sizeof(int));
            }
        }

        close(pipefl[0]);
        close(pipetr[1]);
        wait(0);
        exit(0);
        }
        else{   /* no read from left */
            close(pipefl[0]);
            exit(0);
        }
}

int main(int argc, char **argv){
    int i, pipefd[2];   /* pipefd used for souce process to construct pipeline */
    pipe(pipefd);
    if(fork() == 0){    /* pipeline */
        sieve(pipefd); 
    }
    /* source process */
    close(pipefd[0]);
    for(i = 2; i <= 35; i++){
        write(pipefd[1], &i ,sizeof(int));
    }
    close(pipefd[1]);
    wait(0);
    /* wait until all children are reaped */  
    exit(0); 
}