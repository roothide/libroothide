#ifndef VROOT_INTERNAL_H
#define VROOT_INTERNAL_H

#ifndef VROOT_INTERNAL

#define MACRO_if #if
#define MACRO_else #else
#define MACRO_elif #elif
#define MACRO_endif #endif
#define MACRO_ifdef #ifdef
#define MACRO_ifndef #ifndef
#define MACRO_elifdef #elifdef
#define MACRO_elifndef #elifndef
#define MACRO_define #define

#define VROOT_API_DEF(RET,NAME,ARGTYPES) MACRO_define NAME VROOT_API_NAME(NAME)
#define VROOTAT_API_DEF(RET,NAME,ARGTYPES) MACRO_define NAME VROOTAT_API_NAME(NAME)
#define VROOT_API_WRAP(RET,NAME,ARGTYPES,ARGS,PATHARG) MACRO_define NAME VROOT_API_NAME(NAME)
#define VROOTAT_API_WRAP(RET,NAME,ARGTYPES,ARGS,FD,PATHARG,ATFLAG) MACRO_define NAME VROOTAT_API_NAME(NAME)

MACRO_ifndef VROOT_H
MACRO_define VROOT_H

#endif /*VROOT_INTERNAL*/

#define VROOT_API_NAME(NAME) vroot_##NAME
#define VROOTAT_API_NAME(NAME) vroot_##NAME

#include <Availability.h>

#if !defined(VROOT_API_ALL) && !defined(__IPHONE_16_0)
#error "ios sdk version too old"
#endif

/* dlfcn.h */
VROOT_API_DEF(void*, dlsym, (void * __handle, const char * __symbol))

VROOT_API_DEF(void*, dlopen, (const char * __path, int __mode))

VROOT_API_DEF(int, dladdr, (const void * addr, Dl_info * info) )

/* spawn.h */
VROOT_API_WRAP(int, posix_spawn, (pid_t * pid, const char * path, const posix_spawn_file_actions_t *file_actions,
    const posix_spawnattr_t * attrp, char *const argv[], char *const envp[]),
    (pid,newpath,file_actions,attrp,argv,envp), path)

VROOT_API_DEF(int, posix_spawnp, (pid_t * pid, const char * path, const posix_spawn_file_actions_t *file_actions,
    const posix_spawnattr_t * attrp, char *const argv[], char *const envp[]))

VROOT_API_WRAP(int, posix_spawn_file_actions_addopen, (posix_spawn_file_actions_t * file_actions,
	   int fildes, const char *restrict path, int oflag, mode_t mode),
       (file_actions,fildes,path,oflag,mode), path)

VROOT_API_WRAP(int, posix_spawn_file_actions_addchdir_np, (posix_spawn_file_actions_t *restrict file_actions,
	   const char *restrict	path), (file_actions,path), path)


/* fcntl.h  */

VROOT_API_DEF(int, open, (const char * path, int flags, ...))

VROOTAT_API_DEF(int, openat, (int fd, const char * path, int flags, ...))

VROOT_API_WRAP(int, creat, (const char * path, int mode), (newpath,mode), path)
VROOTAT_API_DEF(int, fcntl, (int, int, ...))
/* apple private */
VROOT_API_WRAP(int, openx_np, (const char *path, int flags, filesec_t fsec), (newpath, flags, fsec), path)
VROOT_API_DEF(int, open_dprotected_np, (const char *, int, int, int, ...))
//int openat_dprotected_np(int, const char *, int, int, int, ...); //not in dsc?
//int openat_authenticated_np(int, const char *, int, int); //not in dsc?
//

/* unistd.h */
VROOT_API_DEF(char*, getwd, (char *) )
VROOT_API_DEF(char*, getcwd, (char *, size_t) )
//not really fs access, size_t confstr(int, char *, size_t) __DARWIN_ALIAS(confstr);
VROOT_API_WRAP(int, acct, (const char *filename), (newpath), filename)
VROOT_API_WRAP(int, access, (const char * path, int mode), (newpath,mode), path)
VROOT_API_WRAP(int, chdir, (const char * path), (newpath), path)
VROOT_API_WRAP(int, chown, (const char * path,uid_t uid, gid_t gid), (newpath,uid,gid), path)

VROOT_API_WRAP(long, pathconf, (const char * path, int name), (newpath,name), path)
VROOT_API_DEF(int, rmdir, (const char * path)) //dpkg remove packages
VROOT_API_WRAP(int, unlink, (const char * path), (newpath), path)
VROOT_API_WRAP(int, chroot, (const char * path), (newpath), path)
VROOT_API_WRAP(int, lchown, (const char * path,uid_t uid, gid_t gid), (newpath,uid,gid), path)
VROOT_API_DEF(ssize_t, readlink, (const char * path, char* buf, size_t bufsiz))
VROOT_API_WRAP(int, truncate, (const char *path,off_t length), (newpath,length), path)

VROOT_API_DEF(int, link, (const char *name1, const char *name2))

VROOT_API_DEF(int, symlink, (const char *name1, const char *name2))

VROOT_API_WRAP(int, mknod, (const char *path, mode_t mode, dev_t dev), (newpath,mode,dev), path)

VROOT_API_DEF(char*, mkdtemp, (char * path))
VROOT_API_DEF(int, mkstemp, (char * path))
VROOT_API_DEF(int,mkstemps,(char *_template, int suffixlen))
//not really fs access char* mktemp(char * path)
VROOT_API_DEF(int, mkostemp, (char * path, int oflags))
VROOT_API_DEF(int, mkostemps, (char * path, int slen, int oflags))

//only /dev/xxx,not about jbroot/ VROOT_API_WRAP(int, revoke, (const char * path), (newpath), path)
VROOT_API_WRAP(int, undelete, (const char * path), (newpath), path)
//Undefined symbol: _unwhiteout?? doesn't exists in dsc, int     unwhiteout(const char *); //not in dsc?

VROOT_API_WRAP(int,getattrlist,(const char* path, struct attrlist * attrList, void * attrBuf,size_t attrBufSize, unsigned int options), (newpath,attrList,attrBuf,attrBufSize,options), path)

VROOT_API_WRAP(int,setattrlist,(const char* path, struct attrlist * attrList, void * attrBuf,
                                  size_t attrBufSize, unsigned int options), (newpath,attrList,attrBuf,attrBufSize,options), path)

VROOT_API_DEF(int, exchangedata, (const char * path1,const char * path2,unsigned int options))

VROOT_API_WRAP(int,searchfs,(const char * path, struct fssearchblock * searchBlock,unsigned long * numMatches, unsigned int scriptCode,unsigned int options, struct searchstate * state), (newpath,searchBlock,numMatches,scriptCode,options,state), path)

VROOT_API_WRAP(int,fsctl,(const char *path,unsigned long request,void*data,unsigned int options), (newpath,request,data,options), path)

VROOT_API_WRAP(int, sync_volume_np, (const char *path, int flags), (newpath,flags), path)


VROOT_API_WRAP(int, mkpath_np, (const char *path, mode_t omode), (newpath,omode), path)
VROOTAT_API_WRAP(int, mkpathat_np, (int dfd, const char *path, mode_t omode), (dfd, newpath, omode), dfd,path,0)

#if defined(VROOT_API_ALL) || defined(TARGET_OS_IPHONE)
VROOT_API_DEF(int, mkstemp_dprotected_np, (char *path, int dpclass, int dpflags))
#endif
VROOTAT_API_DEF(char*, mkdtempat_np, (int dfd, char *path))
VROOTAT_API_DEF(int, mkstempsat_np, (int dfd, char *path, int slen))
VROOTAT_API_DEF(int,mkostempsat_np,(int dfd,char*path,int slen,int oflags))

VROOT_API_DEF(int, execv, (const char * __path, char * const * __argv))
//VROOT_API_WRAP(int, execv, (const char * __path, char * const * __argv), (newpath,__argv), __path)
VROOT_API_WRAP(int, execve, (const char * __file, char * const * __argv, char * const * __envp), (newpath,__argv,__envp), __file)

VROOT_API_DEF(int, execvP, (const char * __file, const char * __searchpath, char * const * __argv))

VROOT_API_DEF(int, execvp, (const char * __file, char * const * __argv))

VROOT_API_DEF(int, execl, (const char *path, const char *arg0, ...))

VROOT_API_DEF(int, execle, (const char *path, const char *arg0, ... /*, (char *)0, char *const envp[] */))

VROOT_API_DEF(int, execlp, (const char *file, const char *arg0, ...))


/* dirent.h */
VROOT_API_WRAP(DIR*, opendir, (const char *filename), (newpath), filename)
VROOT_API_WRAP(int, scandir, (const char *dirname, struct dirent ***namelist,
    int (*select)(const struct dirent *), int (*compar)(const struct dirent **, const struct dirent **)),
    (newpath,namelist,select,compar), (dirname))
VROOT_API_WRAP(int, scandir_b, (const char *dirname, struct dirent ***namelist,
    int (^select)(const struct dirent *), int (^compar)(const struct dirent **, const struct dirent **)),
    (newpath,namelist,select,compar), (dirname))
VROOT_API_WRAP(DIR *, __opendir2, (const char * name, int flags), (newpath, flags), name)
VROOT_API_DEF(struct dirent*, readdir, (DIR *))
VROOT_API_DEF(int, readdir_r, (DIR *, struct dirent *, struct dirent **))


/* utime.h */
VROOT_API_WRAP(int, utime, (const char *file, const struct utimbuf *timep),(newpath,timep), file)
/* time.h */
VROOT_API_WRAP(int, utimes, (const char *path, const struct timeval *times),(newpath,times), path)
VROOT_API_WRAP(int, lutimes, (const char *path, const struct timeval *times),(newpath,times), path)

/* stdio.h */
VROOT_API_WRAP(FILE*, fopen, (const char* filename, const char* mode), (newpath, mode), filename)
VROOT_API_WRAP(FILE*, freopen, (const char* filename, const char* mode, FILE* fp), (newpath, mode, fp), filename)
VROOT_API_WRAP(FILE*, fopen$DARWIN_EXTSN, (const char * __restrict __filename, const char * __restrict __mode), (newpath,__mode), __filename)
VROOT_API_WRAP(int, remove, (const char * path), (newpath), path)
VROOT_API_DEF(int, rename, (const char *__old, const char *__new))

//not really fs access char *tmpnam(char *);
//not really fs access char* tempnam,(const char *__dir, const char *__prefix);

//popen$DARWIN_EXTSN
//FILE* popen(const char *command, const char *type); //shim in sh instead of here ****************************************

/* sys/stat.h */
VROOT_API_WRAP(int, chmod, (const char *path, mode_t mode), (newpath,mode), path)
VROOT_API_DEF(int, lstat, (const char * path, struct stat *sb) )
VROOT_API_WRAP(int, mkdir, (const char * path, mode_t mode), (newpath,mode), path)
VROOT_API_WRAP(int, mkfifo, (const char *path, mode_t mode), (newpath,mode), path)
VROOT_API_WRAP(int, stat, (const char *path, struct stat *sb), (newpath,sb), path) //name confict with struct ********************
//already defined in unistd.h VROOT_API_WRAP(int, mknod, (const char *path, mode_t mode, dev_t dev), (newpath,mode,dev), path)

VROOTAT_API_WRAP(int, fchmodat, (int fd, const char *path, mode_t mode, int flag), (fd,newpath,mode,flag), fd, path, flag)
VROOTAT_API_DEF(int, fstatat, (int fd, const char *path, struct stat *sb, int flag) )
VROOTAT_API_WRAP(int, mkdirat, (int fd, const char * path, mode_t mode), (fd,newpath,mode), fd, path, 0)
#if defined(VROOT_API_ALL) || __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_16_0
VROOTAT_API_WRAP(int, mkfifoat, (int fd, const char * path, mode_t mode), (fd,newpath,mode), fd, path, 0)
VROOTAT_API_WRAP(int, mknodat, (int fd, const char *path, mode_t mode, dev_t dev), (fd,newpath,mode,dev), fd, path, 0)
#endif

VROOTAT_API_WRAP(int, utimensat, (int fd, const char *path, const struct timespec times[2], int flag), (fd,newpath,times,flag), fd,path,flag)

//file-cmds/chmod.c->modify_file_acl? but its from apple opensource probject, this shim library is just for gnu opensource project
VROOT_API_WRAP(int, chmodx_np, (const char *path, filesec_t fsec), (newpath,fsec), path)

VROOT_API_WRAP(int, chflags, (const char *path, __uint32_t flags), (newpath,flags), path)
VROOT_API_WRAP(int, lchflags, (const char *path, __uint32_t flags), (newpath,flags), path)
VROOT_API_WRAP(int, lchmod, (const char *path, mode_t mode), (newpath,mode), path)
VROOT_API_DEF(int, lstatx_np, (const char *path, struct stat *st, filesec_t fsec) )
VROOT_API_WRAP(int, mkdirx_np, (const char *path, filesec_t fsec), (newpath, fsec), path)
VROOT_API_WRAP(int, mkfifox_np, (const char *path, filesec_t fsec), (newpath, fsec), path)
VROOT_API_WRAP(int, statx_np, (const char *path, struct stat *st, filesec_t fsec), (newpath, st, fsec), path)
VROOT_API_WRAP(int, lstat64, (const char *path, struct stat64 *st), (newpath, st), path)
VROOT_API_WRAP(int, stat64, (const char *path, struct stat64 *st), (newpath, st), path)


/* stdlib.h */
//int system(const char *) __DARWIN_ALIAS_C(system); //shim in sh instead of here *******************************
VROOT_API_DEF(char*, realpath, (const char * path, char *resolved_path))
VROOT_API_DEF(char*, realpath$DARWIN_EXTSN, (const char * path, char *resolved_path))
//already defined in unistd.h char* mktemp(char *path)
//already defined in unistd.h int mkstemp(char *path)

/* sys/unistd.h */

VROOTAT_API_WRAP(int,getattrlistat,(int fd,const char* path, struct attrlist * attrList, void * attrBuf,size_t attrBufSize, unsigned int options), (fd,newpath,attrList,attrBuf,attrBufSize,options), fd,path,0)

VROOTAT_API_WRAP(int,setattrlistat,(int fd,const char* path, struct attrlist * attrList, void * attrBuf,
                                  size_t attrBufSize, unsigned int options), (fd,newpath,attrList,attrBuf,attrBufSize,options), fd,path,0)
#if defined(VROOT_API_ALL) || __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_16_0
VROOT_API_DEF(ssize_t, freadlink, (int fd, char* buf, size_t bufsize))
#endif

VROOTAT_API_WRAP(int, faccessat, (int fd, const char *path, int mode, int flag), (fd,newpath,mode,flag), fd, path,flag)
VROOTAT_API_WRAP(int, fchownat, (int fd, const char *path, uid_t owner, gid_t group, int flag), (fd,newpath,owner,group,flag), fd, path,flag)
VROOTAT_API_DEF(ssize_t, readlinkat, (int fd,const char* path,char* buf,size_t bufsize))
VROOTAT_API_WRAP(int, unlinkat, (int dfd, const char *path, int flag), (dfd,newpath,flag), dfd, path,flag)

VROOTAT_API_DEF(int, linkat, (int fd1, const char *name1, int fd2, const char *name2, int flag))

VROOTAT_API_DEF(int, symlinkat, (const char *name1, int fd, const char *name2))


/* mount.h */
VROOT_API_WRAP(int, mount, (const char *type, const char *dir, int flags, void *data), (type,newpath,flags,data), dir)
//??int fmount, (const char *type, int fd, int flags, void *data);
VROOT_API_WRAP(int, statfs, (const char * path, struct statfs *buf), (newpath, buf), path) //name confict with struct********************
VROOT_API_WRAP(int, statfs64, (const char * path, struct statfs64 *buf), (newpath, buf), path)
VROOT_API_WRAP(int, unmount, (const char *dir, int flags), (newpath,flags), dir)
//not a path,int getvfsbyname(const char *fsname, struct vfsconf *vfcp);

VROOT_API_DEF(int, getmntinfo, (struct statfs **mntbufp, int mode)) // __DARWIN_INODE64(getmntinfo);
VROOT_API_DEF(int, getmntinfo_r_np, (struct statfs **mntbufp, int mode)) //__DARWIN_INODE64(getmntinfo_r_np)
VROOT_API_DEF(int, getfsstat, (struct statfs *buf, int bufsize, int mode)) //__DARWIN_INODE64(getfsstat);
VROOT_API_DEF(int, getfsstat64, (struct statfs64 *buf, int bufsize, int mode)) //__OSX_AVAILABLE_BUT_DEPRECATED(__MAC_10_5, __MAC_10_6, __IPHONE_NA, __IPHONE_NA);

/* mman.h */
VROOT_API_DEF(int, shm_open, (const char * path, int flags, ...))

VROOT_API_WRAP(int, shm_unlink, (const char * path), (newpath), path)

/* semaphore */
VROOT_API_DEF(sem_t*, sem_open, (const char *path, int flags, ...))

VROOT_API_WRAP(int, sem_unlink, (const char * path), (newpath), path)

/* copyfile.h */
VROOT_API_DEF(int, copyfile, (const char * from, const char * to, copyfile_state_t state, copyfile_flags_t flags))

/* sys/stdio.h */
VROOTAT_API_DEF(int, renameat, (int fromfd, const char *from, int tofd, const char *to))

VROOT_API_DEF(int, renamex_np, (const char *__old, const char *__new, unsigned int flags))

VROOTAT_API_DEF(int, renameatx_np, (int fromfd, const char *from, int tofd, const char *to, unsigned int flags))

/* db.h */
VROOT_API_WRAP(DB*, dbopen, (const char *file,int flags,int mode,DBTYPE type,const void *openinfo),(newpath,flags,mode,type,openinfo),file)
//__DBINTERFACE_PRIVATE
VROOT_API_WRAP(DB*,__bt_open,(const char *file,int flags,int mode,const BTREEINFO *openinfo,int dflags),(newpath,flags,mode,openinfo,dflags),file)
VROOT_API_WRAP(DB*,__hash_open,(const char *file, int flags, int mode, const HASHINFO *info, int dflags),(newpath,flags,mode,info,dflags),file)
VROOT_API_WRAP(DB*,__rec_open,(const char *file, int flags, int mode, const RECNOINFO *info, int dflags),(newpath,flags,mode,info,dflags),file)

/* sys/statvfs.h */
VROOT_API_WRAP(int, statvfs,(const char * path, struct statvfs * st), (newpath,st), path)


/* c++ fstream */

//filebuf::open(char*,int)
VROOT_API_WRAP(void*, _ZNSt3__113basic_filebufIcNS_11char_traitsIcEEE4openEPKcj,(void* thiz,const char* __s,unsigned int mode),(thiz,newpath,mode), __s)
//ifstream::open(char*,int)
VROOT_API_WRAP(void*, _ZNSt3__114basic_ifstreamIcNS_11char_traitsIcEEE4openEPKcj,(void* thiz,const char* __s,unsigned int mode),(thiz,newpath,mode), __s)
//ifstream::open(std::string,int)
VROOT_API_DEF(void*, _ZNSt3__114basic_ifstreamIcNS_11char_traitsIcEEE4openERKNS_12basic_stringIcS2_NS_9allocatorIcEEEEj,(void* thiz,void* __s,unsigned int mode))
//ofstream::open(char*,int)
VROOT_API_WRAP(void*, _ZNSt3__114basic_ofstreamIcNS_11char_traitsIcEEE4openEPKcj,(void* thiz,const char* __s,unsigned int mode),(thiz,newpath,mode), __s)
//ofstream::open(std::string,int)
VROOT_API_DEF(void*, _ZNSt3__114basic_ofstreamIcNS_11char_traitsIcEEE4openERKNS_12basic_stringIcS2_NS_9allocatorIcEEEEj,(void* thiz,void* __s,unsigned int mode))

/* sys/socket.h */
VROOT_API_DEF(int, bind, (int sockfd, const struct sockaddr *addr, socklen_t addrlen))
VROOT_API_DEF(int, connect, (int sockfd, const struct sockaddr *addr, socklen_t addrlen))
VROOT_API_DEF(int, getpeername,(int sockfd, struct sockaddr *addr, socklen_t* addrlen))
VROOT_API_DEF(int, getsockname, (int sockfd, struct sockaddr *addr, socklen_t* addrlen))


/* ftw.h */
VROOT_API_DEF(int, ftw, (const char *, int (*)(const char *, const struct stat *, int), int) )
VROOT_API_DEF(int, nftw, (const char *, int (*)(const char *, const struct stat *, int, struct FTW *), int, int) )

/* fts.h */
VROOT_API_DEF(FTS*, fts_open, (char * const *, int, int (*)(const FTSENT **, const FTSENT **) ) )
VROOT_API_DEF(FTS*, fts_open_b, (char * const *, int, int (^)(const FTSENT **, const FTSENT **) ) )

/* sys/xattr.h */
VROOT_API_WRAP(ssize_t, getxattr, (const char *path, const char *name, void *value, size_t size, u_int32_t position, int options), (newpath,name,value,size,position,options), path)
VROOT_API_WRAP(int, setxattr, (const char *path, const char *name, const void *value, size_t size, u_int32_t position, int options), (newpath,name,value,size,position,options), path)
VROOT_API_WRAP(int, removexattr, (const char *path, const char *name, int options), (newpath,name,options), name)
VROOT_API_WRAP(ssize_t, listxattr, (const char *path, char *namebuff, size_t size, int options), (newpath,namebuff,size,options), path)

/* glob.h */
VROOT_API_DEF(int, glob, (const char * pattern, int flags, int (* errfunc) (const char *, int), glob_t * pglob))
VROOT_API_DEF(int, glob_b, (const char * pattern, int flags, int (^ errfunc) (const char *, int), glob_t * pglob))

/* sysdir.h */
//not really fs access, sysdir_search_path_enumeration_state sysdir_get_next_search_path_enumeration(sysdir_search_path_enumeration_state state, char *path);

/* dyld.h */
VROOT_API_DEF(int, _NSGetExecutablePath, (char* buf, uint32_t* bufsize))

/* sys/acl.h */
VROOT_API_WRAP(int, acl_valid_file_np, (const char *path, acl_type_t type, acl_t acl), (newpath,type,acl), path)
//not in dsc, VROOT_API_WRAP(int, acl_valid_link_np, (const char *path, acl_type_t type, acl_t acl), (newpath,type,acl), path)
VROOT_API_WRAP(int, acl_delete_def_file, (const char *path_p), (newpath), path_p) /* not supported */
VROOT_API_WRAP(acl_t, acl_get_file, (const char *path_p, acl_type_t type), (newpath,type), path_p)
VROOT_API_WRAP(acl_t, acl_get_link_np, (const char *path_p, acl_type_t type), (newpath,type), path_p)
VROOT_API_WRAP(int, acl_set_file, (const char *path_p, acl_type_t type, acl_t acl), (newpath,type,acl), path_p)
VROOT_API_WRAP(int, acl_set_link_np, (const char *path_p, acl_type_t type, acl_t acl), (newpath,type,acl), path_p)

/* nl_types.h */
VROOT_API_WRAP(nl_catd, catopen, (char *name, int flag), (newpath,flag), name)

/* unicode/urename.h */
//includes u_catopen, ures_open*, But this seems to be Apple's private library\
 and procursus also provides libicu so we won't deal with it for now

/* libxml/xxx */
//procursus provides libxml so we won't deal with it for now


#ifndef VROOT_INTERNAL
MACRO_endif /* VROOT_H */
#endif

#endif /* VROOT_INTERNAL_H */
