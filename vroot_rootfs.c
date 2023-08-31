#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/syslimits.h>

#include "common.h"
#include "libroothide.h"

#define ROOTFS_DIR_NAME "rootfs"

#define VROOT_API_NAME(NAME) vroot_##NAME
#define VROOTAT_API_NAME(NAME) vroot_##NAME

struct dirent* VROOT_API_NAME(readdir)(DIR* dir)
{
    struct dirent* ret = readdir(dir);
    if(ret) {
        
        if(ret->d_type==DT_LNK && strcmp(ret->d_name, ROOTFS_DIR_NAME)==0)
        {
        
            int fd = dirfd(dir);
            struct stat st;
            fstat(fd, &st);
            
            struct stat jbrootst;
            stat(jbroot("/"), &jbrootst);
            
            if(st.st_dev==jbrootst.st_dev
               && st.st_ino==jbrootst.st_ino)
            {
                struct stat rootst;
                stat("/", &rootst);
                
                ret->d_type = DT_DIR;
                ret->d_ino = rootst.st_ino;
            }
        
        }
        
        if(ret->d_type==DT_DIR)
        {
            struct stat st;
            int fd = dirfd(dir);
            fstatat(fd, ret->d_name, &st, 0);
            
            struct stat jbrootst;
            stat(jbroot("/"), &jbrootst);
            
            if(st.st_dev==jbrootst.st_dev
               && st.st_ino==jbrootst.st_ino)
            {
                ret->d_type = DT_LNK;
            }
        }
        
    }
    return ret;
}

int VROOT_API_NAME(readdir_r)(DIR* dir, struct dirent* d, struct dirent ** dp)
{
    int ret = readdir_r(dir,d,dp);
    if(ret == 0) {
        
        if((*dp)->d_type==DT_LNK && strcmp((*dp)->d_name, ROOTFS_DIR_NAME)==0)
        {
            int fd = dirfd(dir);
            struct stat st;
            fstat(fd, &st);
            
            struct stat jbrootst;
            stat(jbroot("/"), &jbrootst);
            
            if(st.st_dev==jbrootst.st_dev
               && st.st_ino==jbrootst.st_ino)
            {
                struct stat rootst;
                stat("/", &rootst);
                
                (*dp)->d_type = DT_DIR;
                (*dp)->d_ino = rootst.st_ino;
            }
            
        }
        
        if((*dp)->d_type==DT_DIR)
        {
            struct stat st;
            int fd = dirfd(dir);
            fstatat(fd, (*dp)->d_name, &st, 0);
            
            struct stat jbrootst;
            stat(jbroot("/"), &jbrootst);
            
            if(st.st_dev==jbrootst.st_dev
               && st.st_ino==jbrootst.st_ino)
            {
                (*dp)->d_type = DT_LNK;
            }
        }
    }
    return ret;
}

int VROOT_API_NAME(lstat)(const char * path, struct stat *sb)
{
    const char* newpath = jbroot_alloc(path);
    int ret = lstat(newpath, sb);
    free((void*)newpath);
    if(ret==0) {
        if(S_ISLNK(sb->st_mode)) {
            struct stat st;
            lstat(jbroot("/"ROOTFS_DIR_NAME), &st);
            if(st.st_dev==sb->st_dev
               && st.st_ino==sb->st_ino)
            {
                struct stat rootst;
                stat("/", &rootst);
                
                sb->st_dev = rootst.st_dev;
                sb->st_ino = rootst.st_ino;
                sb->st_mode &= ~S_IFLNK;
                sb->st_mode |= S_IFDIR;
            }
        }
        
        if(S_ISDIR(sb->st_mode)) {
            struct stat st;
            stat(jbroot("/"), &st);
            if(st.st_dev==sb->st_dev
               && st.st_ino==sb->st_ino)
            {
                char bname[PATH_MAX];
                if(is_jbroot_name(basename_r(path,bname))) {
                    sb->st_mode &= ~S_IFDIR;
                    sb->st_mode |= S_IFLNK;
                }
            }
        }
    }
    return ret;
}

int VROOT_API_NAME(lstatx_np)(const char *path, struct stat *sb, filesec_t fsec)
{
    const char* newpath = jbroot_alloc(path);
    int ret = lstatx_np(newpath, sb, fsec);
    free((void*)newpath);
    if(ret==0) {
        if(S_ISLNK(sb->st_mode)) {
            struct stat st;
            lstat(jbroot("/"ROOTFS_DIR_NAME), &st);
            if(st.st_dev==sb->st_dev
               && st.st_ino==sb->st_ino)
            {
                struct stat rootst;
                stat("/", &rootst);
                
                sb->st_dev = rootst.st_dev;
                sb->st_ino = rootst.st_ino;
                sb->st_mode &= ~S_IFLNK;
                sb->st_mode |= S_IFDIR;
            }
        }
        
        if(S_ISDIR(sb->st_mode)) {
            struct stat st;
            stat(jbroot("/"), &st);
            if(st.st_dev==sb->st_dev
               && st.st_ino==sb->st_ino)
            {
                char bname[PATH_MAX];
                if(is_jbroot_name(basename_r(path,bname))) {
                    sb->st_mode &= ~S_IFDIR;
                    sb->st_mode |= S_IFLNK;
                }
            }
        }
    }
    return ret;
}


int VROOT_API_NAME(fstatat)(int fd, const char *path, struct stat *sb, int flag)
{
    const char* newpath = jbrootat_alloc(fd, path);
    int ret = fstatat(fd, newpath, sb, flag);
    if(ret ==0 )
    {
        //if(flag & AT_SYMLINK_NOFOLLOW)
        if(S_ISLNK(sb->st_mode)) {
            struct stat st;
            lstat(jbroot("/"ROOTFS_DIR_NAME), &st);
            if(st.st_dev==sb->st_dev
               && st.st_ino==sb->st_ino)
            {
                struct stat rootst;
                stat("/", &rootst);
                
                sb->st_dev = rootst.st_dev;
                sb->st_ino = rootst.st_ino;
                sb->st_mode &= ~S_IFLNK;
                sb->st_mode |= S_IFDIR;
            }
        }
        
        if(S_ISDIR(sb->st_mode)) {
            struct stat st;
            stat(jbroot("/"), &st);
            if(st.st_dev==sb->st_dev
               && st.st_ino==sb->st_ino)
            {
                char bname[PATH_MAX];
                if(is_jbroot_name(basename_r(path,bname))) {
                    sb->st_mode &= ~S_IFDIR;
                    sb->st_mode |= S_IFLNK;
                }
            }
        }
    }
    return ret;
}

ssize_t VROOTAT_API_NAME(readlinkat)(int fd,const char* path,char* buf,size_t bufsize)
{
    const char* newpath = jbrootat_alloc(fd, path);
    
    struct stat sb;
    if(path[strlen(path)] != '/' && //EINVAL
       fstatat(fd, newpath, &sb, AT_SYMLINK_NOFOLLOW)==0) {
        if(S_ISLNK(sb.st_mode)) {
            struct stat st;
            lstat(jbroot("/rootfs"), &st);
            if(st.st_dev==sb.st_dev
               && st.st_ino==sb.st_ino)
            {
                //gnu realpath always readlink each subpath
                free((void*)newpath);
                errno = EINVAL;
                return -1;
            }
        }
        
        if(S_ISDIR(sb.st_mode)) {

                struct stat st;
                stat(jbroot("/"), &st);
                if(st.st_dev==sb.st_dev
                   && st.st_ino==sb.st_ino)
                {
                    char bname[PATH_MAX];
                    if(is_jbroot_name(basename_r(path,bname))) {
                        free((void*)newpath);
                        if(bufsize>0) {
                            buf[0] = '/';
                            return 1;
                        } {
                            return 0;
                        }
                    }
                }
        }
    }
                
    ssize_t ret = readlinkat(fd, newpath, buf, bufsize);
    if(newpath) free((void*)newpath);
    if(ret > 0) {
        char* linkbuf = malloc(bufsize+1);
        memcpy(linkbuf, buf, bufsize);
        linkbuf[ret] = '\0';
        const char* newlink = rootfs_alloc(linkbuf);
        free((void*)linkbuf);
        memset(buf, 0, ret); //don't modify
        size_t len = strlen(newlink);
        if(len > bufsize) len = bufsize;
        memcpy(buf, newlink, len);
        free((void*)newlink);
        return len;
    }
    return ret;
}

ssize_t VROOT_API_NAME(readlink)(const char * path, char* buf, size_t bufsiz)
{
    return VROOT_API_NAME(readlinkat)(AT_FDCWD, path, buf, bufsiz);
}
