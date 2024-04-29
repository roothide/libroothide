#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>
#include <mach-o/dyld.h>

#include "common.h"
#include "roothide.h"

#define VROOT_API_NAME(NAME) vroot_##NAME
#define VROOTAT_API_NAME(NAME) vroot_##NAME


EXPORT
void* VROOT_API_NAME(dlopen)(const char * __path, int __mode)
{
VROOT_LOG("@%s %s %x\n",__FUNCTION__, __path, __mode);

    if(__path && _dyld_shared_cache_contains_path(__path)) {
        return dlopen(__path, __mode);
    }

    const char* newpath = jbroot_alloc(__path);
    void* ret = dlopen(newpath, __mode);
    if(newpath) free((void*)newpath);
    return ret;
}

EXPORT
int VROOT_API_NAME(dladdr)(const void * addr, Dl_info * info)
{
VROOT_LOG("@%s %p\n",__FUNCTION__, addr);

    int ret = dladdr(addr, info);
    if(ret != 0 && !_dyld_shared_cache_contains_path(info->dli_fname))
    {
        //need use cache
        const char* newfname = rootfs(info->dli_fname);
        if(strcmp(info->dli_fname, newfname) != 0)
        {
            info->dli_fname = newfname;
        }

        //sname??? fakechroot do this, do we need?
//        const char* newsname = jbroot(info->dli_sname);
//        if(strcmp(info->dli_sname, newsname) != 0)
//        {
//            info->dli_sname = newsname;
//        }
    }
    return ret;
}

extern char* gvrootsymnames[];
extern void* gvrootsymaddrs[];
extern void* gvrootapiaddrs[];

EXPORT
void* VROOT_API_NAME(dlsym)(void * __handle, const char * __symbol)
{
VROOT_LOG("@%s %p %s\n",__FUNCTION__, __handle, __symbol);

    void* ret = dlsym(__handle, __symbol);
    for(int i=0; gvrootsymaddrs[i]; i++) {
        if(ret == gvrootsymaddrs[i] && strcmp(__symbol, gvrootsymnames[i])==0) {
            ret = gvrootapiaddrs[i];
            break;
        }
    }

    return ret;
}