#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define READ_END 0
#define WRITE_END 1


/*
* good job, girl!
*/
__attribute__((noreturn)) 
void handle(int read_fd){
    int cur_proc_num = -1;
    int num;
    int forked = 0;
    int my_pipe[2];
    pipe(my_pipe);
    while(read(read_fd, &num, 4) > 0){
        if(cur_proc_num == -1){
            cur_proc_num = num;
            printf("prime %d\n", cur_proc_num);
        } else if(num % cur_proc_num != 0) {
            if(!forked){
                forked = 1;
                if(fork() == 0){
                    close(my_pipe[WRITE_END]);
                    handle(my_pipe[READ_END]);
                } else {
                    close(my_pipe[READ_END]);
                }
            }
            write(my_pipe[WRITE_END], &num, 4);
        }
    }
    close(my_pipe[WRITE_END]);
    close(read_fd);
    if(forked){
        int status;
        wait(&status);
    }
    exit(0);
}
int main(int argc, char *argv[]){
    int my_pipe[2];
    pipe(my_pipe);
    if(fork() == 0){
        close(my_pipe[WRITE_END]);
        handle(my_pipe[READ_END]);
        exit(0);
    } else {
        for(int i = 2; i <= 35; ++i){
            write(my_pipe[WRITE_END], &i, 4);
        }
    }
    close(my_pipe[WRITE_END]);
    int status;
    wait(&status);
    exit(0);
}