#include "roothide.h"
#include "common.h"

EXPORT
NSString* __attribute__((overloadable)) jbroot(NSString* path)
{
    const char* _p = jbroot_alloc(path.UTF8String);
    if(!_p) return path;
    NSString* newpath = [NSString stringWithUTF8String:_p];
    free((void*)_p);
    return newpath;
}

EXPORT
NSString* __attribute__((overloadable)) rootfs(NSString* path)
{
    const char* _p = rootfs_alloc(path.UTF8String);
    if(!_p) return path;
    NSString* newpath = [NSString stringWithUTF8String:_p];
    free((void*)_p);
    return newpath;
}
