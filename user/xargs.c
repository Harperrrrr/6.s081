#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    char *nargv[argc + 1];
    for(int i = 0; i < argc - 1; ++i){
        nargv[i] = argv[i + 1];
    }

    char buf[512];
    char *p = buf;
    int status;
    while(read(0, p, 1) > 0){
        if(*p == '\n'){
            *p = 0;
            if(fork() == 0){
                nargv[argc - 1] = buf;
                nargv[argc] = 0;
                exec(nargv[0], nargv);
                exit(0);
            } else {
                wait(&status);
                p = buf;
                continue;
            }
        }
        p++;
    }
    close(0);
    exit(0);
}