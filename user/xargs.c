#include "../kernel/types.h"
#include "../kernel/stat.h"
#include "../user/user.h"
#include "../kernel/param.h"
#define DEBUG 1

int main(int argc, char **argv){
    int i;
    char *xargv[MAXARG];   /* MAXARG define in param.h */
    char line[512], **p, *end;
    
    /* copy argument from argv to xargv */
    p = xargv;
    for(i = 1; argv[i]; i++){
        *p = argv[i];
        p++;
    }

    end = line;
    while(read(0, end, 1) > 0){
        if(*end != '\n'){
            end++;
        }
        else{
            *end = '\0';
            *p = line;
            end = line;
            if(fork() == 0){
                exec(argv[1], xargv);
                exit(0); /* exec failed */
            }
            wait(0);
        }
    } 
    exit(0);
}