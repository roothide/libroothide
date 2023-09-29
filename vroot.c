#include <unistd.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <spawn.h>
#include <utime.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>
#include <copyfile.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/fcntl.h>
#include <sys/semaphore.h>
#include <sys/statvfs.h>
#include <sys/xattr.h>
#include <sys/socket.h>
#include <sys/un.h>
#define __DBINTERFACE_PRIVATE
#include <db.h>
#include <ftw.h>
#include <fts.h>
#include <glob.h>
#include <mach-o/dyld.h>
#include <libgen.h>

#include "roothide.h"
#include "common.h"

#define jbrootat_alloc(a,b,c) jbrootat_alloc(a,b)

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"


#define EXPORT __attribute__ ((visibility ("default")))

#define VROOT_API_DEF(RET,NAME,ARGTYPES) EXPORT RET vroot_##NAME ARGTYPES ;
#define VROOTAT_API_DEF(RET,NAME,ARGTYPES) EXPORT RET vroot_##NAME ARGTYPES ;

#define VROOT_API_WRAP(RET,NAME,ARGTYPES,ARGS,PATHARG) EXPORT RET vroot_##NAME ARGTYPES {\
VROOT_LOG("*vroot_%s %s\n", #NAME, PATHARG);\
    int olderr = errno;\
    char* newpath = (char*)jbroot_alloc(PATHARG);\
    errno = olderr;\
    RET ret = NAME ARGS;\
    if(newpath) free((void*)newpath);\
    return ret;\
}

#define VROOTAT_API_WRAP(RET,NAME,ARGTYPES,ARGS,FD,PATHARG,ATFLAG) EXPORT RET vroot_##NAME ARGTYPES {\
VROOT_LOG("*vrootat_%s %d %08X %s\n", #NAME, FD, ATFLAG, PATHARG);\
    int olderr = errno;\
    char* newpath = (char*)jbrootat_alloc(FD,PATHARG,ATFLAG);\
    errno = olderr;\
    RET ret = NAME ARGS;\
    if(newpath) free((void*)newpath);\
    return ret;\
}

//force define
char* realpath(const char * path, char *resolved_path);
char* realpath$DARWIN_EXTSN(const char * path, char *resolved_path);
FILE* fopen(const char * __restrict __filename, const char * __restrict __mode);
FILE* fopen$DARWIN_EXTSN(const char * __restrict __filename, const char * __restrict __mode);

int open$NOCANCEL(const char * path, int flags, ...);
int openat$NOCANCEL(int fd, const char * path, int flags, ...);
int creat$NOCANCEL(const char * path, int mode);
int fcntl$NOCANCEL(int, int, ...);
int system$NOCANCEL(const char *);

struct stat64 __DARWIN_STRUCT_STAT64;
int lstat64(const char *, struct stat64 *);
int stat64(const char *, struct stat64 *);

struct statfs64 __DARWIN_STRUCT_STATFS64;
int statfs64(const char *, struct statfs64 *);
int getfsstat64(struct statfs64 *, int, int);

//c++
void* _ZNSt3__113basic_filebufIcNS_11char_traitsIcEEE4openEPKcj(void* thiz, const char* __s, unsigned int mode);
void* _ZNSt3__114basic_ifstreamIcNS_11char_traitsIcEEE4openEPKcj(void* thiz, const char* __s, unsigned int mode);
void* _ZNSt3__114basic_ifstreamIcNS_11char_traitsIcEEE4openERKNS_12basic_stringIcS2_NS_9allocatorIcEEEEj(void* thiz, void* __s, unsigned int mode);
void* _ZNSt3__114basic_ofstreamIcNS_11char_traitsIcEEE4openEPKcj(void* thiz, const char* __s, unsigned int mode);
void* _ZNSt3__114basic_ofstreamIcNS_11char_traitsIcEEE4openERKNS_12basic_stringIcS2_NS_9allocatorIcEEEEj(void* thiz, void* __s, unsigned int mode);

int oflag_to_atflag(int oflag)
{
    int atflag=0;
    if(oflag & O_NOFOLLOW) atflag |= AT_SYMLINK_NOFOLLOW;
    if(oflag & O_NOFOLLOW_ANY) atflag |= AT_SYMLINK_NOFOLLOW_ANY;
    return atflag;
}

#define VROOT_INTERNAL
#include "vroot.h"


int VROOT_API_NAME(open)(const char * path, int flags, ...)
{
VROOT_LOG("@%s %08X %s\n",__FUNCTION__, flags, path);

    mode_t mode = 0;
    va_list ap;
    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);
    const char* newpath = jbrootat_alloc(AT_FDCWD, path, oflag_to_atflag(flags));
    int ret = open(newpath, flags, mode);
    
    if(ret<0 && errno==ENOTDIR && (flags&O_NOFOLLOW)) {
        struct stat sb;
        if(lstat(newpath, &sb)==0) {
            struct stat st={0};
            char buf[PATH_MAX];
            snprintf(buf,sizeof(buf),"%s"ROOTFS_NAME, jbroot("/"));
            lstat(buf, &st);
            if(st.st_dev==sb.st_dev
               && st.st_ino==sb.st_ino)
            {
                ret = open(newpath, flags&~O_NOFOLLOW, mode);
            }
        }
    }
    
    if(newpath) free((void*)newpath);
    return ret;
}

int VROOT_API_NAME(openat)(int fd, const char * path, int flags, ...)
{
VROOT_LOG("@%s %d %08X %s\n",__FUNCTION__, fd, flags, path);

    mode_t mode = 0;
    va_list ap;
    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);
    const char* newpath = jbrootat_alloc(fd, path, oflag_to_atflag(flags));
    int ret = openat(fd, newpath, flags, mode);
    
    if(ret<0 && errno==ENOTDIR && (flags&O_NOFOLLOW)) {
        struct stat sb;
        if(fstatat(fd, newpath, &sb, AT_SYMLINK_NOFOLLOW)==0) {
            struct stat st={0};
            char buf[PATH_MAX];
            snprintf(buf,sizeof(buf),"%s"ROOTFS_NAME, jbroot("/"));
            lstat(buf, &st);
            if(st.st_dev==sb.st_dev
               && st.st_ino==sb.st_ino)
            {
                ret = openat(fd, newpath, flags&~O_NOFOLLOW, mode);
            }
        }
    }
    
    if(newpath) free((void*)newpath);
    return ret;
}

int VROOT_API_NAME(open_dprotected_np)(const char *path, int flags, int class, int dpflags, ...)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    mode_t mode = 0;
    va_list ap;
    va_start(ap, dpflags);
    mode = va_arg(ap, int);
    va_end(ap);
    const char* newpath = jbrootat_alloc(AT_FDCWD, path, oflag_to_atflag(flags));
    int ret = open_dprotected_np(newpath, flags, class, dpflags, mode);
    if(newpath) free((void*)newpath);
    return ret;
}

int VROOT_API_NAME(link)(const char *name1, const char *name2)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    const char* newname1 = jbroot_alloc(name1);
    const char* newname2 = jbroot_alloc(name2);
    int ret = link(newname1, newname2);
    if(newname1) free((void*)newname1);
    if(newname2) free((void*)newname2);
    return ret;
}

int VROOT_API_NAME(symlink)(const char *name1, const char *name2)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    int ret = 0;
    const char* newname2 = jbroot_alloc(name2);

    char dname[PATH_MAX];
    int dfd = open(dirname_r(newname2, dname), O_RDONLY);
    if(dfd >= 0)
    {
        const char* newname1 = jbrootat_alloc(dfd, name1, 0);
        ret = symlink(newname1, newname2);
        if(newname1) free((void*)newname1);

        close(dfd);

    } else {
        ret = -1;
    }

    if(newname2) free((void*)newname2);

    return ret;
}

int VROOT_API_NAME(rename)(const char *__old, const char *__new)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    const char* new__old = jbroot_alloc(__old);
    const char* new__new = jbroot_alloc(__new);
    int ret = rename(new__old, new__new);
    if(new__old) free((void*)new__old);
    if(new__new) free((void*)new__new);
    return ret;
}

int VROOTAT_API_NAME(linkat)(int fd1, const char *name1, int fd2, const char *name2, int flag)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    /* flag is for name1 and only support AT_SYMLINK_FOLLOW, but its not about find path */
    const char* newname1 = jbrootat_alloc(fd1, name1, 0);
    const char* newname2 = jbrootat_alloc(fd2, name2, 0);
    int ret = linkat(fd1, newname1, fd2, newname2, flag);
    if(newname1) free((void*)newname1);
    if(newname2) free((void*)newname2);
    return ret;
}

int VROOTAT_API_NAME(symlinkat)(const char *name1, int fd, const char *name2)
{
VROOT_LOG("@%s %s %d %s\n",__FUNCTION__, name1, fd, name2);

    int ret = 0;
    const char* newname2 = jbrootat_alloc(fd, name2, 0); //***********

    char dname[PATH_MAX];
    int dfd = openat(fd, dirname_r(newname2, dname), O_RDONLY);
    if(dfd >= 0)
    {
        const char* newname1 = jbrootat_alloc(dfd, name1, 0);
        int ret = symlinkat(newname1, fd, newname2);
        if(newname1) free((void*)newname1);
        
        close(dfd);
    } else {
        ret = -1;
    }

    if(newname2) free((void*)newname2);
    return ret;
}

int VROOT_API_NAME(shm_open)(const char * path, int flags, ...)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    mode_t mode = 0;
    va_list ap;
    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);
    const char* newpath = jbroot_alloc(path);
    int ret = shm_open(newpath, flags, mode);
    if(newpath) free((void*)newpath);
    return ret;
}

sem_t* VROOT_API_NAME(sem_open)(const char *path, int flags, ...)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    mode_t mode = 0;
    va_list ap;
    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);
    const char* newpath = jbroot_alloc(path);
    sem_t* ret = sem_open(newpath, flags, mode);
    if(newpath) free((void*)newpath);
    return ret;
}

int VROOT_API_NAME(copyfile)(const char * from, const char * to, copyfile_state_t state, copyfile_flags_t flags)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    const char* newfrom = jbroot_alloc(from);
    const char* newto = jbroot_alloc(to);
    int ret = copyfile(newfrom, newto, state, flags);
    if(newfrom) free((void*)newfrom);
    if(newto) free((void*)newto);
    return ret;
}

int VROOTAT_API_NAME(renameat)(int fromfd, const char *from, int tofd, const char *to)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    const char* newfrom = jbrootat_alloc(fromfd, from, 0);
    const char* newto = jbrootat_alloc(tofd, to, 0);
    int ret = renameat(fromfd, newfrom, tofd, newto);
    if(newfrom) free((void*)newfrom);
    if(newto) free((void*)newto);
    return ret;
}

int VROOT_API_NAME(renamex_np)(const char *__old, const char *__new, unsigned int flags)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    const char* new__old = jbroot_alloc(__old);
    const char* new__new = jbroot_alloc(__new);
    int ret = renamex_np(new__old, new__new, flags);
    if(new__old) free((void*)new__old);
    if(new__new) free((void*)new__new);
    return ret;
}

int VROOTAT_API_NAME(renameatx_np)(int fromfd, const char *from, int tofd, const char *to, unsigned int flags)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    const char* newfrom = jbrootat_alloc(fromfd, from, flags);
    const char* newto = jbrootat_alloc(tofd, to, flags);
    int ret = renameatx_np(fromfd, newfrom, tofd, newto, flags);
    if(newfrom) free((void*)newfrom);
    if(newto) free((void*)newto);
    return ret;
}

int VROOT_API_NAME(exchangedata)(const char * path1,const char * path2,unsigned int options)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    const char* newpath1 = jbroot_alloc(path1);
    const char* newpath2 = jbroot_alloc(path2);
    int ret = exchangedata(newpath1, newpath2, options);
    if(newpath1) free((void*)newpath1);
    if(newpath2) free((void*)newpath2);
    return ret;
}

char* VROOT_API_NAME(realpath)(const char * path, char *resolved_path)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    char pathbuf[PATH_MAX]={0};
    const char* newpath = jbroot_alloc(path);
    char* ret = realpath(newpath, pathbuf);

    //ret = pathbuf or NULL
    if(ret || resolved_path)
    {
        const char* rp = rootfs_alloc(pathbuf);
        //if(!rp)
////        {
//            FILE* fp = fopen("/var/revert.log", "a");
//            fprintf(fp, "path=%p,%s newpath=%p,%s pathbuf=%s rp=%p,%s ret=%p,arg=%p\n", path,path, newpath,newpath, pathbuf, rp,rp,  ret,resolved_path);
//            fclose(fp);
////        }

        if(resolved_path) {
            strncpy(resolved_path, rp, PATH_MAX);
            if(ret) ret = resolved_path;
        }
        else if(ret) {
            ret = strdup(rp);
        }

        free((void*)rp);
    }

    if(newpath) free((void*)newpath);

    return ret;
}

char* VROOT_API_NAME(realpath$DARWIN_EXTSN)(const char * path, char *resolved_path)
{
VROOT_LOG("@%s %s\n",__FUNCTION__, path);

    char pathbuf[PATH_MAX]={0};
    const char* newpath = jbroot_alloc(path);
    char* ret = realpath$DARWIN_EXTSN(newpath, pathbuf);

    if(ret || resolved_path)
    {
        const char* rp = rootfs_alloc(pathbuf);
        //if(!rp)
//        {
//            FILE* fp = fopen("/var/revert.log", "a");
//            fprintf(fp, "path=%p,%s newpath=%p,%s pathbuf=%s rp=%p,%s ret=%p,arg=%p\n", path,path, newpath,newpath, pathbuf, rp,rp,  ret,resolved_path);
//            fclose(fp);
//        }

        if(resolved_path) {
            strncpy(resolved_path, rp, PATH_MAX);
            if(ret) ret = resolved_path;
        }
        else if(ret) {
            ret = strdup(rp);
        }

        free((void*)rp);
    }
    
    if(newpath) free((void*)newpath);
    
    return ret;
}

/* xun-source/libsyscall/wrappers/cancelable/fcntl-base.c */
#define F_OPENFROM      56              /* SPI: open a file relative to fd (must be a dir) */
#define F_UNLINKFROM    57              /* SPI: open a file relative to fd (must be a dir) */
#define F_CHECK_OPENEVT 58              /* SPI: if a process is marked OPENEVT, or in O_EVTONLY on opens of this vnode */
#define F_OFD_SETLK             90      /* Acquire or release open file description lock */
#define F_OFD_SETLKW            91      /* (as F_OFD_SETLK but blocking if conflicting lock) */
#define F_OFD_GETLK             92      /* Examine OFD lock */
#define F_OFD_SETLKWTIMEOUT     93      /* (as F_OFD_SETLKW but return if timeout) */
#define F_OFD_GETLKPID          94      /* get record locking information */
#define F_SETCONFINED           95      /* "confine" OFD to process */
#define F_GETCONFINED           96      /* is-fd-confined? */
int VROOT_API_NAME(fcntl)(int fd, int cmd, ...)
{
VROOT_LOG("@%s %d\n",__FUNCTION__, cmd);

    va_list ap;
    void *arg;

    va_start(ap, cmd);
    switch (cmd) {
    case F_GETLK:
    case F_GETLKPID:
    case F_SETLK:
    case F_SETLKW:
    case F_SETLKWTIMEOUT:
    case F_OFD_GETLK:
    case F_OFD_GETLKPID:
    case F_OFD_SETLK:
    case F_OFD_SETLKW:
    case F_OFD_SETLKWTIMEOUT:
    case F_PREALLOCATE:
    case F_PUNCHHOLE:
    case F_SETSIZE:
    case F_RDADVISE:
    case F_LOG2PHYS:
    case F_LOG2PHYS_EXT:
    case F_GETPATH:
    case F_GETPATH_NOFIRMLINK:
    case F_GETPATH_MTMINFO:
    case F_GETCODEDIR:
    case F_PATHPKG_CHECK:
    case F_OPENFROM:
    case F_UNLINKFROM:
    case F_ADDSIGS:
    case F_ADDFILESIGS:
    case F_ADDFILESIGS_FOR_DYLD_SIM:
    case F_ADDFILESIGS_RETURN:
    case F_ADDFILESIGS_INFO:
    case F_ADDFILESUPPL:
    case F_FINDSIGS:
    case F_TRANSCODEKEY:
    case F_TRIM_ACTIVE_FILE:
    case F_SPECULATIVE_READ:
    case F_CHECK_LV:
    case F_GETSIGSINFO:
        arg = va_arg(ap, void *);
        break;
    default:
        arg = (void *)((unsigned long)va_arg(ap, int));
        break;
    }
    va_end(ap);
    
    int ret = fcntl(fd, cmd, arg);
    
    if(ret == 0)
    {
        if(cmd == F_GETPATH)
        { //buf size is always PATH_MAX
            char* pathbuf = arg;
            if(pathbuf) {
                const char* rp = rootfs_alloc(pathbuf);
                strncpy(pathbuf, rp, PATH_MAX);
                if(rp) free((void*)rp);
            }
        }
    }
    
    return ret;
}

int VROOT_API_NAME(bind)(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    struct sockaddr_un *addr_un = (struct sockaddr_un *)(addr);
    if (addr_un->sun_family == AF_UNIX && addr_un->sun_path[0]) {
        socklen_t newaddrlen;
        struct sockaddr_un newaddr_un;

        
        const int af_unix_path_max = sizeof(addr_un->sun_path);
        const char *path = addr_un->sun_path;

        const char* newpath = jbroot_alloc(path);
        
        memset(&newaddr_un, 0, sizeof(struct sockaddr_un));
        newaddr_un.sun_family = addr_un->sun_family;
        strlcpy(newaddr_un.sun_path, path, sizeof(newaddr_un.sun_path));
        
        free((void*)newpath);
        
        if (strlen(path) >= af_unix_path_max) {
            errno = (ENAMETOOLONG);
            return -1;
        }
        
        addrlen = (socklen_t)SUN_LEN(&newaddr_un);
        addr = (struct sockaddr *)&newaddr_un;
    }
    
    return bind(sockfd, addr, addrlen);
}

int VROOT_API_NAME(connect)(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    struct sockaddr_un *addr_un = (struct sockaddr_un *)(addr);
    if (addr_un->sun_family == AF_UNIX && addr_un->sun_path[0]) {
        socklen_t newaddrlen;
        struct sockaddr_un newaddr_un;

        
        const int af_unix_path_max = sizeof(addr_un->sun_path);
        const char *path = addr_un->sun_path;

        const char* newpath = jbroot_alloc(path);
        
        memset(&newaddr_un, 0, sizeof(struct sockaddr_un));
        newaddr_un.sun_family = addr_un->sun_family;
        strlcpy(newaddr_un.sun_path, path, sizeof(newaddr_un.sun_path));
        
        free((void*)newpath);
        
        if (strlen(path) >= af_unix_path_max) {
            errno = (ENAMETOOLONG);
            return -1;
        }
        
        addrlen = (socklen_t)SUN_LEN(&newaddr_un);
        addr = (struct sockaddr *)&newaddr_un;
    }
    
    return connect(sockfd, addr, addrlen);
}

int VROOT_API_NAME(getpeername)(int sockfd, struct sockaddr *addr, socklen_t* addrlen)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    socklen_t origlen = *addrlen;
    
    int ret = getpeername(sockfd, addr, addrlen);
    
    struct sockaddr_un *addr_un = (struct sockaddr_un *)(addr);
    if (addr_un->sun_family == AF_UNIX && addr_un->sun_path[0]) {

        size_t path_max = origlen - offsetof(struct sockaddr_un, sun_path);
        if (path_max > sizeof(addr_un->sun_path)) {
            path_max = sizeof(addr_un->sun_path);
        }
        if (addr_un->sun_path[0]) {
            const char* newpath = rootfs_alloc(addr_un->sun_path);
            strlcpy(addr_un->sun_path, newpath, path_max);
            *addrlen = (socklen_t)SUN_LEN(addr_un);
            free((void*)newpath);
        }
    }
    
    return ret;
}
int VROOT_API_NAME(getsockname)(int sockfd, struct sockaddr *addr, socklen_t* addrlen)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    socklen_t origlen = *addrlen;
    
    int ret = getpeername(sockfd, addr, addrlen);
    
    struct sockaddr_un *addr_un = (struct sockaddr_un *)(addr);
    if (addr_un->sun_family == AF_UNIX && addr_un->sun_path[0]) {

        size_t path_max = origlen - offsetof(struct sockaddr_un, sun_path);
        if (path_max > sizeof(addr_un->sun_path)) {
            path_max = sizeof(addr_un->sun_path);
        }
        if (addr_un->sun_path[0]) {
            const char* newpath = rootfs_alloc(addr_un->sun_path);
            strlcpy(addr_un->sun_path, newpath, path_max);
            *addrlen = (socklen_t)SUN_LEN(addr_un);
            free((void*)newpath);
        }
    }
    
    return ret;
}

char* VROOT_API_NAME(getwd)(char* buf)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    char* ret = getwd(buf);
    if(ret) {
        const char* newpath = rootfs_alloc(ret);
        size_t len = strlcpy(buf, newpath, PATH_MAX);
        free((void*)newpath);
        if(len >= PATH_MAX) {
            errno = ENAMETOOLONG;
            return NULL;
        }
    }
    return ret;
}

char* VROOT_API_NAME(getcwd)(char *buf, size_t bufsize)
{
VROOT_LOG("@%s\n",__FUNCTION__);
    //the caller buf may too small for real fs path???
    char* ret = getcwd(buf, bufsize);
    if(ret) {
        const char* newpath = rootfs_alloc(ret);
        size_t len = strlcpy(buf, newpath, bufsize);
        free((void*)newpath);
        if(len >= PATH_MAX) {
            errno = ERANGE;
            return NULL;
        }
    }
    return ret;
}

int VROOT_API_NAME(glob)(const char * pattern, int flags, int (* errfunc) (const char *, int), glob_t * pglob)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    int ret = glob(pattern,flags,errfunc,pglob);
    if(ret == 0) {
        for(int i=0; i<pglob->gl_pathc; i++)
        {
            const char* newpath = rootfs_alloc(pglob->gl_pathv[i]);
            if(strcmp(newpath, pglob->gl_pathv[i]) != 0)
            {
                free((void*)pglob->gl_pathv[i]);
                pglob->gl_pathv[i] = (char*)newpath;
                
            } else {
                free((void*)newpath);
            }
        }
    }
    return ret;
}

int VROOT_API_NAME(glob_b)(const char * pattern, int flags, int (^ errfunc) (const char *, int), glob_t * pglob)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    int ret = glob_b(pattern,flags,errfunc,pglob);
    if(ret == 0) {
        for(int i=0; i<pglob->gl_pathc; i++)
        {
            const char* newpath = rootfs_alloc(pglob->gl_pathv[i]);
            if(strcmp(newpath, pglob->gl_pathv[i]) != 0)
            {
                free((void*)pglob->gl_pathv[i]);
                pglob->gl_pathv[i] = (char*)newpath;
                
            } else {
                free((void*)newpath);
            }
        }
    }
    return ret;
}

int __thread (*ftw_callback)(const char *fpath, const struct stat *sb, int typeflag);
int ftw_function(const char *fpath, const struct stat *sb, int typeflag)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    const char* newpath = rootfs_alloc(fpath);
    int ret = ftw_callback(newpath,sb,typeflag);
    free((void*)newpath);
    return ret;
}
int VROOT_API_NAME(ftw)(const char *dirpath, int (*fn)(const char *fpath, const struct stat *sb, int typeflag), int nopenfd)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    ftw_callback = fn;
    
    const char* newpath = jbroot_alloc(dirpath);
    int ret = ftw(newpath, ftw_function, nopenfd);
    free((void*)newpath);
    return ret;
}

int __thread (*nftw_callback)(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
int nftw_function(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    const char* newpath = rootfs_alloc(fpath);
    int ret = nftw_callback(newpath,sb,typeflag,ftwbuf);
    free((void*)newpath);
    return ret;
}
int VROOT_API_NAME(nftw)(const char *dirpath,
                          int (*fn)(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf), int nopenfd, int flags)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    nftw_callback = fn;
    
    const char* newpath = jbroot_alloc(dirpath);
    int ret = nftw(newpath, nftw_function, nopenfd, flags);
    free((void*)newpath);
    return ret;
}

/* paths in FTSENT are always subpath component of path_argv[i], so we don't need to convert them */
FTS* VROOT_API_NAME(fts_open)(char * const *path_argv, int options, int (*compar)(const FTSENT **, const FTSENT **) )
{
VROOT_LOG("@%s\n",__FUNCTION__);

    int pathc = 0;
    for(int i=0; path_argv[i]; i++)
        pathc++;
    
    char** newpathv = malloc(sizeof(path_argv[0]) * (pathc+1));
    for(int i=0; path_argv[i]; i++) {
        newpathv[i] = (char*)jbroot_alloc(path_argv[i]);
    }
    newpathv[pathc] = NULL;
    
    FTS* ret = fts_open(newpathv, options, compar);
    
    for(int i=0; newpathv[i]; i++) {
        free((void*)newpathv[i]);
    }
    free((void*)newpathv);
    return ret;
}

FTS* VROOT_API_NAME(fts_open_b)(char * const *path_argv, int options, int (^compar)(const FTSENT **, const FTSENT **) )
{
VROOT_LOG("@%s\n",__FUNCTION__);

    int pathc = 0;
    for(int i=0; path_argv[i]; i++)
        pathc++;
    
    char** newpathv = malloc(sizeof(path_argv[0]) * (pathc+1));
    for(int i=0; path_argv[i]; i++) {
        newpathv[i] = (char*)jbroot_alloc(path_argv[i]);
    }
    newpathv[pathc] = NULL;
    
    FTS* ret = fts_open_b(newpathv, options, compar);
    
    for(int i=0; newpathv[i]; i++) {
        free((void*)newpathv[i]);
    }
    free((void*)newpathv);
    return ret;
}

int VROOT_API_NAME(getmntinfo)(struct statfs **mntbufp, int mode)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    int ret = getmntinfo(mntbufp, mode);
    if(ret > 0) for(int i=0; i<ret; i++) {
        const char* newfrom = rootfs_alloc((*mntbufp)[i].f_mntfromname);
        const char* newto = rootfs_alloc((*mntbufp)[i].f_mntonname);
        strncpy((*mntbufp)[i].f_mntfromname, newfrom, sizeof((*mntbufp)[i].f_mntfromname));
        strncpy((*mntbufp)[i].f_mntonname, newto, sizeof((*mntbufp)[i].f_mntonname));
        free((void*)newfrom);
        free((void*)newto);
    }
    return ret;
}
int VROOT_API_NAME(getmntinfo_r_np)(struct statfs **mntbufp, int mode)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    int ret = getmntinfo_r_np(mntbufp, mode);
    if(ret > 0) for(int i=0; i<ret; i++) {
        const char* newfrom = rootfs_alloc((*mntbufp)[i].f_mntfromname);
        const char* newto = rootfs_alloc((*mntbufp)[i].f_mntonname);
        strncpy((*mntbufp)[i].f_mntfromname, newfrom, sizeof((*mntbufp)[i].f_mntfromname));
        strncpy((*mntbufp)[i].f_mntonname, newto, sizeof((*mntbufp)[i].f_mntonname));
        free((void*)newfrom);
        free((void*)newto);
    }
    return ret;
}
int VROOT_API_NAME(getfsstat)(struct statfs *buf, int bufsize, int mode)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    int ret = getfsstat(buf, bufsize, mode);
    if(ret>0 && buf) for(int i=0; i<ret; i++) {
        const char* newfrom = rootfs_alloc(buf[i].f_mntfromname);
        const char* newto = rootfs_alloc(buf[i].f_mntonname);
        strncpy(buf[i].f_mntfromname, newfrom, sizeof(buf[i].f_mntfromname));
        strncpy(buf[i].f_mntonname, newto, sizeof(buf[i].f_mntonname));
        free((void*)newfrom);
        free((void*)newto);
    }
    return ret;
}
int VROOT_API_NAME(getfsstat64)(struct statfs64 *buf, int bufsize, int mode)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    int ret = getfsstat64(buf, bufsize, mode);
    if(ret>0 && buf) for(int i=0; i<ret; i++) {
        const char* newfrom = rootfs_alloc(buf[i].f_mntfromname);
        const char* newto = rootfs_alloc(buf[i].f_mntonname);
        strncpy(buf[i].f_mntfromname, newfrom, sizeof(buf[i].f_mntfromname));
        strncpy(buf[i].f_mntonname, newto, sizeof(buf[i].f_mntonname));
        free((void*)newfrom);
        free((void*)newto);
    }
    return ret;
}

int VROOT_API_NAME(rmdir)(const char * path)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    const char* newpath = jbroot_alloc(path);
    
    int olderr = errno;

    //dpkg remove packages
    char* jbrootsymlink=NULL;
    asprintf(&jbrootsymlink, "%s/.jbroot", newpath);
    char lbuf[PATH_MAX]={0};
    if(readlink(jbrootsymlink, lbuf, sizeof(lbuf)-1) && lbuf[0]=='/')
    { //not a bootstrap symlink
        unlink(jbrootsymlink);
    }
    free(jbrootsymlink);

    errno = olderr;
    
    int ret = rmdir(newpath);
    free((void*)newpath);
    return ret;
}

int VROOT_API_NAME(_NSGetExecutablePath)(char* buf, uint32_t* bufsize)
{
VROOT_LOG("@%s\n",__FUNCTION__);

    uint32_t oldsize = *bufsize; 
    int ret = _NSGetExecutablePath(buf, bufsize);
    if(ret == 0) {
        const char* newpath = rootfs_alloc(buf);
        int newsize = strlen(newpath);
        if(newsize < oldsize) {
            strcpy(buf, newpath);
        } else {
            *bufsize = newsize+1;
            ret = -1;
        }
        free((void*)newpath);
    }
    return ret;
}


// symbols array 

char* gvrootsymnames[] = {
#undef VROOT_API_DEF
#undef VROOTAT_API_DEF
#undef VROOT_API_WRAP
#undef VROOTAT_API_WRAP
#define VROOT_API_DEF(RET,NAME,ARGTYPES) #NAME,
#define VROOTAT_API_DEF(RET,NAME,ARGTYPES) #NAME,
#define VROOT_API_WRAP(RET,NAME,ARGTYPES,ARGS,PATHARG) #NAME,
#define VROOTAT_API_WRAP(RET,NAME,ARGTYPES,ARGS,FD,PATHARG,ATFLAG) #NAME,
#define VROOT_INTERNAL
#undef VROOT_INTERNAL_H
#include "vroot.h"
#undef VROOT_INTERNAL_H
#undef VROOT_INTERNAL
#undef VROOT_API_DEF
#undef VROOTAT_API_DEF
#undef VROOT_API_WRAP
#undef VROOTAT_API_WRAP

NULL
};

void* gvrootsymaddrs[] = {
#undef VROOT_API_DEF
#undef VROOTAT_API_DEF
#undef VROOT_API_WRAP
#undef VROOTAT_API_WRAP
#define VROOT_API_DEF(RET,NAME,ARGTYPES) NAME,
#define VROOTAT_API_DEF(RET,NAME,ARGTYPES) NAME,
#define VROOT_API_WRAP(RET,NAME,ARGTYPES,ARGS,PATHARG) NAME,
#define VROOTAT_API_WRAP(RET,NAME,ARGTYPES,ARGS,FD,PATHARG,ATFLAG) NAME,
#define VROOT_INTERNAL
#undef VROOT_INTERNAL_H
#include "vroot.h"
#undef VROOT_INTERNAL_H
#undef VROOT_INTERNAL
#undef VROOT_API_DEF
#undef VROOTAT_API_DEF
#undef VROOT_API_WRAP
#undef VROOTAT_API_WRAP

NULL
};


void* gvrootapiaddrs[] = {
#undef VROOT_API_DEF
#undef VROOTAT_API_DEF
#undef VROOT_API_WRAP
#undef VROOTAT_API_WRAP
#define VROOT_API_DEF(RET,NAME,ARGTYPES) vroot_##NAME,
#define VROOTAT_API_DEF(RET,NAME,ARGTYPES) vroot_##NAME,
#define VROOT_API_WRAP(RET,NAME,ARGTYPES,ARGS,PATHARG) vroot_##NAME,
#define VROOTAT_API_WRAP(RET,NAME,ARGTYPES,ARGS,FD,PATHARG,ATFLAG) vroot_##NAME,
#define VROOT_INTERNAL
#undef VROOT_INTERNAL_H
#include "vroot.h"
#undef VROOT_INTERNAL_H
#undef VROOT_INTERNAL
#undef VROOT_API_DEF
#undef VROOTAT_API_DEF
#undef VROOT_API_WRAP
#undef VROOTAT_API_WRAP

NULL
};
