#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include "roothide.h"
#include "common.h"

/*
 file system w^x restricted, double jbroot structure.
 all relative path symlinks(include .jbroot) in the directories
 in this list will be processed as absolute paths based on the original rootfs
  at the first time we updatelink. also need to be defined in updatelinks.sh
*/
const char* jbfirmlinks[] = {
    "/var"
};

int main(int argc, const char * argv[]) {
    
    struct stat jbsymst;
    assert(lstat(jbroot("/"), &jbsymst) == 0);
    
    struct stat rootst;
    assert(lstat("/", &rootst) == 0);
    
    char path[PATH_MAX];
    while(fgets(path, sizeof(path), stdin)) {
        size_t len = strlen(path);
        if(len && path[len-1]=='\n') path[len-1]='\0';
        
        const char* lpath = jbroot(path);
        
        struct stat st;
        assert(lstat(lpath, &st) == 0);
        
        //don't change jbroot symlink
        if(st.st_dev==jbsymst.st_dev && st.st_ino==jbsymst.st_ino) {
            printf("jbroot symlink!\n");
            continue;
        }
        
        if(st.st_dev==rootst.st_dev && st.st_ino==rootst.st_ino) {
            printf("rootfs symlink!\n");
            continue;
        }
        
        char slink[PATH_MAX]={0}; //readlink not padding with \0
        assert(readlink(lpath, slink, sizeof(slink)-1)>0);
        
        printf("%s: %s\n", path, slink);
        
        char* tojbroot = &slink[sizeof(JB_ROOT_SYM)-1];
        if(strncmp(slink, JB_ROOT_SYM, sizeof(JB_ROOT_SYM)-1)==0
           && (*tojbroot=='/' || *tojbroot=='\0'))
        {
            const char* newpath = jbroot(tojbroot);
            
            // .jbroot links only exists in bootstrap and them should not move to others place, so do we need update them?
            // and updatelink need .jbroot to load dependence library nexttime
//            assert(unlink(jbpath(path)) == 0);
//            assert(symlink(newpath, jbpath(path)) == 0);
            
            printf(".jbroot link => %s\n", newpath);
            
        }
        else if(slink[0] == '/')
        {
            const char* newpath = rootfs(slink);
            if(strncmp(newpath, "/rootfs/", sizeof("/rootfs/")-1)==0) {
                printf("not in jbroot\n");
                continue;
            }

            newpath = jbroot(newpath);
            
            if(strcmp(newpath, slink)==0) {
                printf("no change\n");
                continue;
            }
            
            assert(unlink(lpath) == 0);
            assert(symlink(newpath, lpath) == 0);
            
            printf("update => %s\n", newpath);
        }
        else
        {
            printf("relative path link\n");
        }
        
    }
    
    return 0;
}
