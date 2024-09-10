//
// Created by Ruiting Shi on 2024/9/7.
//

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define READ_END 0
#define WRITE_END 1
int main(int argc, char *argv[]){
    int pipe1[2];
    int pipe2[2];
    pipe(pipe1);
    pipe(pipe2);
    if (fork() == 0){
        uint8 byte;
        close(pipe1[WRITE_END]);
        read(pipe1[READ_END], &byte, 1);
        printf("%d: received ping\n", getpid());
        close(pipe1[READ_END]);

        close(pipe2[READ_END]);
        write(pipe2[WRITE_END], &byte, 1);
        close(pipe2[WRITE_END]);
        exit(0);
    } else {
        uint8 byte = 'c';
        close(pipe1[READ_END]);
        write(pipe1[WRITE_END], &byte, 1);
        close(pipe1[WRITE_END]);

        close(pipe2[WRITE_END]);
        read(pipe2[READ_END], &byte, 1);
        printf("%d: received pong\n", getpid());
        close(pipe2[READ_END]);
        int status;
        wait(&status);
    }
    exit(0);
}