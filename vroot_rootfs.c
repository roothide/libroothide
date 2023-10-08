#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/syslimits.h>

#include "common.h"
#include "roothide.h"

#define VROOT_API_NAME(NAME) vroot_##NAME
#define VROOTAT_API_NAME(NAME) vroot_##NAME

EXPORT
struct dirent* VROOT_API_NAME(readdir)(DIR* dir)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    struct dirent* ret = readdir(dir);
    if(ret) {
        
        if(ret->d_type==DT_LNK && strcmp(ret->d_name, ROOTFS_NAME)==0)
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

EXPORT
int VROOT_API_NAME(readdir_r)(DIR* dir, struct dirent* d, struct dirent ** dp)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    int ret = readdir_r(dir,d,dp);
    if(ret == 0) {
        
        if((*dp)->d_type==DT_LNK && strcmp((*dp)->d_name, ROOTFS_NAME)==0)
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

EXPORT
int VROOT_API_NAME(lstat)(const char * path, struct stat *sb)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    int olderr = errno;
    const char* newpath = jbroot_alloc(path);
    errno = olderr;
    int ret = lstat(newpath, sb);
     olderr = errno;
    if(ret==0) {
        if(S_ISLNK(sb->st_mode)) {
            struct stat st={0};
            char buf[PATH_MAX];
            snprintf(buf,sizeof(buf),"%s"ROOTFS_NAME, jbroot("/"));
            lstat(buf, &st);
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

            char lbuf[PATH_MAX]={0};
            int lsize = readlink(newpath, lbuf, sizeof(lbuf)-1);
            const char* newlink = rootfs_alloc(lbuf);
            sb->st_size = strlen(newlink);
            free((void*)newlink);
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

    free((void*)newpath);
    errno = olderr;
    return ret;
}

EXPORT
int VROOT_API_NAME(lstatx_np)(const char *path, struct stat *sb, filesec_t fsec)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    const char* newpath = jbroot_alloc(path);
    int ret = lstatx_np(newpath, sb, fsec);

    if(ret==0) {
        if(S_ISLNK(sb->st_mode)) {
            struct stat st={0};
            char buf[PATH_MAX];
            snprintf(buf,sizeof(buf),"%s"ROOTFS_NAME, jbroot("/"));
            lstat(buf, &st);
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

            char lbuf[PATH_MAX]={0};
            int lsize = readlink(newpath, lbuf, sizeof(lbuf)-1);
            const char* newlink = rootfs_alloc(lbuf);
            sb->st_size = strlen(newlink);
            free((void*)newlink);
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

    free((void*)newpath);

    return ret;
}

EXPORT
int VROOT_API_NAME(fstatat)(int fd, const char *path, struct stat *sb, int flag)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    const char* newpath = jbrootat_alloc(fd, path);

    int ret = fstatat(fd, newpath, sb, flag);

    if(ret ==0 )
    {
        //if(flag & AT_SYMLINK_NOFOLLOW)
        if(S_ISLNK(sb->st_mode)) {
            struct stat st={0};
            char buf[PATH_MAX];
            snprintf(buf,sizeof(buf),"%s"ROOTFS_NAME, jbroot("/"));
            lstat(buf, &st);
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

            char lbuf[PATH_MAX]={0};
            int lsize = readlinkat(fd, newpath, lbuf, sizeof(lbuf)-1);
            const char* newlink = rootfs_alloc(lbuf);
            sb->st_size = strlen(newlink);
            free((void*)newlink);
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

    free((void*)newpath);

    return ret;
}

EXPORT
ssize_t VROOTAT_API_NAME(readlinkat)(int fd,const char* path,char* buf,size_t bufsize)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    const char* newpath = jbrootat_alloc(fd, path);
    
    struct stat sb;
    if(path[strlen(path)] != '/' && //EINVAL
       fstatat(fd, newpath, &sb, AT_SYMLINK_NOFOLLOW)==0) {
        if(S_ISLNK(sb.st_mode)) {
            struct stat st={0};
            char buf[PATH_MAX];
            snprintf(buf,sizeof(buf),"%s"ROOTFS_NAME, jbroot("/"));
            lstat(buf, &st);
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
                
    char mybuf[PATH_MAX]={0}; //the caller buf may too small for real fs link
    ssize_t ret = readlinkat(fd, newpath, mybuf, sizeof(mybuf)-1);
    if(newpath) free((void*)newpath);
    if(ret > 0) {
        assert(ret < sizeof(mybuf));
        const char* newlink = rootfs_alloc(mybuf);
        size_t len = strlen(newlink);
        if(len > bufsize) len = bufsize;
        memcpy(buf, newlink, len);
        free((void*)newlink);
        return len;
    }
    return ret;
}

EXPORT
ssize_t VROOT_API_NAME(readlink)(const char * path, char* buf, size_t bufsiz)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    return VROOT_API_NAME(readlinkat)(AT_FDCWD, path, buf, bufsiz);
}
