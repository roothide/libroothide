#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include "roothide.h"

int main(int argc, const char * argv[])
{
    if(strcmp(getprogname(),"jbrand")==0) {
        printf("%16llX", jbrand());
    }
    else if(strcmp(getprogname(),"jbroot")==0) {
        if(argc > 2) {
            fprintf(stderr, "wrong arg count:%d\n", argc);
            return -1;
        }
        const char* path = argc==2 ? argv[1] : "/";
        printf("%s", jbroot(path));

    } else if(strcmp(getprogname(),"rootfs")==0) {
        if(argc != 2) {
            fprintf(stderr, "wrong arg count:%d\n", argc);
            return -1;
        }
        const char* path = argv[1];
        printf("%s", jbroot(path));
    } else {
        fprintf(stderr, "wrong arg progname:%s\n", getprogname());
        return -1;
    }
    return 0;
}