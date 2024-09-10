#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path, char *file_name){
    int fd;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
    }

    if(st.type != T_DIR){
        fprintf(2, "find: %s is not a directory\n", path);
    }

    struct dirent de;
    char buf[512], *p;
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if(de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0){
            continue;
        }

        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if(stat(buf, &st) < 0){
            printf("ls: cannot stat %s\n", buf);
            continue;
        }

        if(st.type == T_FILE && strcmp(de.name, file_name) == 0){
            printf("%s\n", buf);
        }

        if(st.type == T_DIR){
            find(buf, file_name);
        }
    }


}
int main(int argc, char *argv[]){
    if (argc < 3) {
        fprintf(2, "find: param error\n");
    }
    find(argv[1], argv[2]);
    exit(0);
}