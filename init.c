#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libgen.h>
#include <dlfcn.h>
#include <sys/param.h>

#include "common.h"

EXPORT
const char* __roothideinit_JBRAND=NULL;
EXPORT
const char* __roothideinit_JBROOT=NULL;

#define BOOTSTRAP_LIBRARY_PATH     "/usr/lib/roothideinit.dylib"

int bootstrap()
{
    struct dl_info di={0};
    assert(dladdr((void*)bootstrap, &di) != 0);

    char* librealpath = strdup(di.dli_fname);

    assert(strlen(librealpath) >= (sizeof(BOOTSTRAP_LIBRARY_PATH)-1));

    char* plib = librealpath + strlen(librealpath) - (sizeof(BOOTSTRAP_LIBRARY_PATH)-1);
    assert(strcmp(plib, BOOTSTRAP_LIBRARY_PATH) == 0);

    *plib = '\0';
    const char* jbroot = librealpath;
    
    char bname[PATH_MAX];
    basename_r(jbroot, bname);

    assert(is_jbroot_name(bname));

    uint64_t randvalue = resolve_jbrand_value(bname);
    assert(randvalue != 0);

    char jbrand[32]={0};
    snprintf(jbrand, sizeof(jbrand), "%016llX", randvalue);

    __roothideinit_JBRAND = strdup(jbrand);
    __roothideinit_JBROOT = strdup(jbroot);

    free(librealpath);
    return 0;
}

static void __attribute__((__constructor__)) _roothide_init()
{
    assert(bootstrap() == 0);
}