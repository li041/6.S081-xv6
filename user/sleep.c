#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char **argv){
    if(argc != 2){
        fprintf(2, "%s: missing operand\n", argv[0]);
        exit(1);
    }
    int time = atoi(argv[1]);
    if(argv[1] == 0){
        fprintf(2, "%s: invalid time interval '%s'", argv[0], argv[1]); 
        exit(1);
    }
    sleep(time);
    exit(0);
}