#include "common.h"
#include "roothide.h"
#include "cache.h"
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/syslimits.h>

#define JBRAND  __roothideinit_JBRAND
#define JBROOT  __roothideinit_JBROOT

EXPORT
unsigned long long jbrand() {
    return strtoull(JBRAND, NULL, 16);
}

//free after use
static const char* __private_jbrootat_alloc(int fd, const char* path)
{
    int olderr = errno;
    
    char atdir[PATH_MAX]={0};
    fd==AT_FDCWD ? (long)getcwd(atdir,sizeof(atdir)) : fcntl(fd, F_GETPATH, atdir);
    JBPATH_LOG(" **jbrootat_alloc (%d)%s %s\n", fd, atdir, path);
    
    if(!path || !*path) {
        errno = olderr;
        return NULL;
    }

    struct stat jbrootst;
    assert(stat(JBROOT, &jbrootst) == 0);
    
    struct stat rootst;
    assert(stat("/", &rootst) == 0);
    
    
    //
    char resolved[PATH_MAX];
    
    struct stat sb;
    char *p, *s;
    size_t left_len, resolved_len;
    char left[PATH_MAX], next_token[PATH_MAX];

    
    resolved[0] = '\0';
    resolved_len = 0;
    left_len = strlcpy(left, path, sizeof(left));
    
    if (left_len >= sizeof(left) || resolved_len >= PATH_MAX) {
        errno = ENAMETOOLONG;
        return (NULL);
    }

    /*
     * Iterate over path components in `left'.
     */
    while (left_len != 0) {
        /*
         * Extract the next path component and adjust `left'
         * and its length.
         */
        p = strchr(left, '/');
        s = p ? p : left + left_len;
        if (s - left >= sizeof(next_token)) {
            errno = ENAMETOOLONG;
            return (NULL);
        }
        memcpy(next_token, left, s - left);
        next_token[s - left] = '\0';
        left_len -= s - left;
        if (p != NULL)
            memmove(left, s + 1, left_len + 1);
        if (resolved_len>0 && resolved[resolved_len - 1] != '/')
        {
            if (resolved_len + 1 >= PATH_MAX) {
                errno = ENAMETOOLONG;
                return (NULL);
            }
            resolved[resolved_len++] = '/';
            resolved[resolved_len] = '\0';
        }
        if (next_token[0] == '\0') {
            strcpy(next_token, "/");
        }

        //check if next token is ..
        if(strcmp(next_token, "..")==0)
        {
            //jbroot is always readable when jailbroken
            int jbrootfd = open(JBROOT, O_RDONLY);
            assert(jbrootfd >= 0);
            
            int checkfd = fd;
            char* checkpath = resolved;
            
            if(path[0] == '/') {
                checkfd = jbrootfd;
                while(*checkpath == '/') checkpath++;
            }
            if(*checkpath=='\0') checkpath = ".";
            //resolved path is always in jbroot-dir
            if (fstatat(checkfd, checkpath, &sb, 0) == 0) {
                //check if current path is jbroot-dir
                if(sb.st_ino==jbrootst.st_ino
                   && sb.st_dev==jbrootst.st_dev)
                {
                    snprintf(next_token, sizeof(next_token), ".");
                }
                else if(sb.st_dev==rootst.st_dev
                       && sb.st_ino==rootst.st_ino)
                { /* jbroot:/rootfs/../ => abspath jbroot:/ */
                    resolved[0] = '\0';
                    resolved_len = 0;
                    strcpy(next_token, "/");
                }
            }
            
            close(jbrootfd);
            
        }
        
       resolved_len = strlcat(resolved, next_token, PATH_MAX);
       if (resolved_len >= PATH_MAX) {
           errno = ENAMETOOLONG;
           return (NULL);
       }
        
        
    } //end while

    /*
     * Remove trailing slash except when the resolved pathname
     * is a single "/".
     */
    if (resolved_len > 1 && resolved[resolved_len - 1] == '/')
        resolved[resolved_len - 1] = '\0';
    
    
    JBPATH_LOG("*resolved:%ld %s\n", resolved_len, resolved);

    errno = olderr;
    return strdup(resolved);
}

//free after use
EXPORT
const char* jbrootat_alloc(int fd, const char* path)
{
    if(!path) return NULL;
    
    const char* fixedpath = __private_jbrootat_alloc(fd, path);
    
    if(!fixedpath) return strdup(path);
    
    // empty or relative path?
    if(fixedpath[0] != '/') return fixedpath;
    
    //its necessary for symlink /rootfs/xxx -> /rootfs/yyy
    if(strlen(fixedpath)>=(sizeof("/"ROOTFS_NAME)-1)
       && strncmp(fixedpath, "/"ROOTFS_NAME, sizeof("/"ROOTFS_NAME)-1)==0)
    {
    //    char atdir[PATH_MAX]={0};
    //    fd==AT_FDCWD ? (long)getcwd(atdir,sizeof(atdir)) : fcntl(fd, F_GETPATH, atdir);
    //    printf(" **rootfs--> (%d)%s\n\t%s : %s\n", fd, atdir, path, fixedpath);
        
        if(fixedpath[sizeof("/"ROOTFS_NAME)-1]=='/') {
            char* newpath = strdup(&fixedpath[sizeof("/"ROOTFS_NAME)-1]);
            free((void*)fixedpath);
            return newpath;
        }
        //break find / and cd rootfs;realpath . ??? caused by lstat(jbroot("/rootfs")) in vroot module
        if(fixedpath[sizeof("/"ROOTFS_NAME)-1]=='\0') {
            free((void*)fixedpath);
            return strdup("/");
        }
    }
    
    size_t pathlen = strlen(JBROOT) + strlen(fixedpath) + 1;
    char* newpath = malloc(pathlen);
    strcpy(newpath, JBROOT);
    strlcat(newpath, fixedpath, pathlen);
    
    free((void*)fixedpath);
    
    return newpath;
}

//free after use
EXPORT
const char* jbroot_alloc(const char* path)
{
    return jbrootat_alloc(AT_FDCWD, path);
}

//free after use
static const char* __private_rootfs_alloc(const char* path)
{
    int olderr = errno;

    JBPATH_LOG(" **rootfs_alloc %s\n", path);
    
    if(!path || !*path) {
        errno = olderr;
        return NULL;
    }

    struct stat jbsympst;
    assert(stat(JB_ROOT_PARENT, &jbsympst) == 0);
    
    int jbroot_based = 0;


    char resolved[PATH_MAX];
    
    struct stat sb;
    char *p, *s;
    size_t left_len, resolved_len;
    char left[PATH_MAX], next_token[PATH_MAX];

    
    resolved[0] = '\0';
    resolved_len = 0;
    left_len = strlcpy(left, path, sizeof(left));
    
    if (left_len >= sizeof(left) || resolved_len >= PATH_MAX) {
        errno = ENAMETOOLONG;
        return (NULL);
    }

    /*
     * Iterate over path components in `left'.
     */
    while (left_len != 0) {
        /*
         * Extract the next path component and adjust `left'
         * and its length.
         */
        p = strchr(left, '/');
        s = p ? p : left + left_len;
        if (s - left >= sizeof(next_token)) {
            errno = ENAMETOOLONG;
            return (NULL);
        }
        memcpy(next_token, left, s - left);
        next_token[s - left] = '\0';
        left_len -= s - left;
        if (p != NULL)
            memmove(left, s + 1, left_len + 1);
        if (resolved_len>0 && resolved[resolved_len - 1] != '/')
        {
            if (resolved_len + 1 >= PATH_MAX) {
                errno = ENAMETOOLONG;
                return (NULL);
            }
            resolved[resolved_len++] = '/';
            resolved[resolved_len] = '\0';
        }
        if (next_token[0] == '\0') {
            strcpy(next_token, "/");
        }

        //check if next token is jb-root-name
        if(is_jbroot_name(next_token)) {
            //hard link not allowed for directory
            if (fstatat(AT_FDCWD, resolved_len?resolved:".", &sb, 0) == 0) {
                //check if current path is jbroot-parent-dir
                if(sb.st_ino==jbsympst.st_ino && sb.st_dev==jbsympst.st_dev)
                {
                    jbroot_based = 1;
                    
                    resolved[0] = '\0';
                    resolved_len = 0;
                    strcpy(next_token, "/");
                    
                    //continue resolve sub path?
                }
            }
        }
        
       resolved_len = strlcat(resolved, next_token, PATH_MAX);
       if (resolved_len >= PATH_MAX) {
           errno = ENAMETOOLONG;
           return (NULL);
       }
        
        
    } //end while

    /*
     * Remove trailing slash except when the resolved pathname
     * is a single "/".
     */
    if (resolved_len > 1 && resolved[resolved_len - 1] == '/')
        resolved[resolved_len - 1] = '\0';
    
    char* retval = NULL;
    if(jbroot_based==0 && path[0] == '/') { //revert a path out of jbroot?
        assert(resolved[0] == '/');
        retval = malloc(sizeof("/"ROOTFS_NAME)-1 + strlen(resolved) + 1);
        strcpy(retval, "/"ROOTFS_NAME);//just add rootfs prefix
        strcat(retval, resolved);
    } else {
        retval = strdup(resolved);
    }
    
    JBPATH_LOG("*resolved:%ld %s\n", resolved_len, retval);

    errno = olderr;
    return retval;
}

//use cache
EXPORT
const char* jbroot(const char* path)
{
    const char* newpath = jbroot_alloc(path);
    
    if(!newpath) return NULL;
    
    static struct cache_path cache;
    const char* cachedpath = cache_path(&cache, newpath);
    
    free((void*)newpath);
    
    return cachedpath;
}

/* free after use */
EXPORT
const char* rootfs_alloc(const char* path)
{
    if(!path) return path;
    
    const char* newpath = __private_rootfs_alloc(path);
    
    if(!newpath) newpath = strdup(path);
    
    return newpath;
}

//use cache
EXPORT
const char* rootfs(const char* path) 
{
    const char* newpath = rootfs_alloc(path);
    
    if(!newpath) return NULL;
    
    static struct cache_path cache;
    const char* cachedpath = cache_path(&cache, newpath);
    
    free((void*)newpath);
    
    return cachedpath;
}
