#ifndef ROOTHIDE_H
#define ROOTHIDE_H

#pragma GCC diagnostic ignored "-Wunused-function"

#include <string.h>

#ifdef __cplusplus
#include <string>
#endif

#ifdef __OBJC__
#import <Foundation/NSString.h>
#endif

//stub functions

#ifdef __cplusplus
extern "C" {
#endif

static const char* rootfs_alloc(const char* path) { return path ? strdup(path) : path; }
static const char* jbroot_alloc(const char* path) { return path ? strdup(path) : path; }
static const char* jbrootat_alloc(int fd, const char* path) { return path ? strdup(path) : path; }

static unsigned long long jbrand() { return 0; }
static const char* jbroot(const char* path) { return path; }
static const char* rootfs(const char* path) { return path; }

#ifdef __OBJC__
static NSString* __attribute__((overloadable)) jbroot(NSString* path) { return path; }
static NSString* __attribute__((overloadable)) rootfs(NSString* path) { return path; }
#endif

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
static std::string jbroot(std::string path) { return path; }
static std::string rootfs(std::string path) { return path; }
#endif

#endif /* ROOTHIDE_H */
