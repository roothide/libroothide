#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <sys/param.h>
#include <mach-o/dyld.h>

#include "common.h"

EXPORT
const char* __roothideinit_JBRAND=NULL;
EXPORT
const char* __roothideinit_JBROOT=NULL;

struct proc_bsdinfo {
	uint32_t                pbi_flags;              /* 64bit; emulated etc */
	uint32_t                pbi_status;
	uint32_t                pbi_xstatus;
	uint32_t                pbi_pid;
	uint32_t                pbi_ppid;
	uid_t                   pbi_uid;
	gid_t                   pbi_gid;
	uid_t                   pbi_ruid;
	gid_t                   pbi_rgid;
	uid_t                   pbi_svuid;
	gid_t                   pbi_svgid;
	uint32_t                rfu_1;                  /* reserved */
	char                    pbi_comm[MAXCOMLEN];
	char                    pbi_name[2 * MAXCOMLEN];  /* empty if no name is registered */
	uint32_t                pbi_nfiles;
	uint32_t                pbi_pgid;
	uint32_t                pbi_pjobc;
	uint32_t                e_tdev;                 /* controlling tty dev */
	uint32_t                e_tpgid;                /* tty process group id */
	int32_t                 pbi_nice;
	uint64_t                pbi_start_tvsec;
	uint64_t                pbi_start_tvusec;
};

#define PROC_PIDTBSDINFO                3
#define PROC_PIDTBSDINFO_SIZE           (sizeof(struct proc_bsdinfo))

int proc_pidinfo(int pid, int flavor, uint64_t arg,  void *buffer, int buffersize) __OSX_AVAILABLE_STARTING(__MAC_10_5, __IPHONE_2_0);

//some process may be killed by sandbox if call systme getppid()
pid_t __getppid()
{
    struct proc_bsdinfo procInfo;
	if (proc_pidinfo(getpid(), PROC_PIDTBSDINFO, 0, &procInfo, sizeof(procInfo)) <= 0) {
		return -1;
	}
    return procInfo.pbi_ppid;
}

#include <pwd.h>
#include <libgen.h>
#include <stdio.h>

#define CONTAINER_PATH_PREFIX   "/private/var/mobile/Containers/Data/" // +/Application,PluginKitPlugin,InternalDaemon

void redirectNSHomeDir(const char* rootdir)
{
    // char executablePath[PATH_MAX]={0};
    // uint32_t bufsize=sizeof(executablePath);
    // if(_NSGetExecutablePath(executablePath, &bufsize)==0 && strstr(executablePath,"testbin2"))
    //     printf("redirectNSHomeDir %s, %s\n\n", rootdir, getenv("CFFIXED_USER_HOME"));

    //for now libSystem should be initlized, container should be set.

    char* homedir = NULL;

/* 
there is a bug in NSHomeDirectory,
if a containerized root process changes its uid/gid, 
NSHomeDirectory will return a home directory that it cannot access. (exclude NSTemporaryDirectory)
We just keep this bug:
*/
    if(!issetugid()) // issetugid() should always be false at this time. (but how about persona-mgmt? idk)
    {
        homedir = getenv("CFFIXED_USER_HOME");
        if(homedir)
        {
            if(strncmp(homedir, CONTAINER_PATH_PREFIX, sizeof(CONTAINER_PATH_PREFIX)-1) == 0)
            {
                return; //containerized
            }
            else
            {
                homedir = NULL; //from parent, drop it
            }
        }
    }

    if(!homedir) {
        struct passwd* pwd = getpwuid(geteuid());
        if(pwd && pwd->pw_dir) {
            homedir = pwd->pw_dir;
        }
    }

    // if(!homedir) {
    //     //CFCopyHomeDirectoryURL does, but not for NSHomeDirectory
    //     homedir = getenv("HOME");
    // }

    if(!homedir) {
        homedir = "/var/empty";
    }

    char newhome[PATH_MAX]={0};
    snprintf(newhome,sizeof(newhome),"%s/%s",rootdir,homedir);
    setenv("CFFIXED_USER_HOME", newhome, 1);
}

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
    const char* JBRAND = getenv("JBRAND");
    const char* JBROOT = getenv("JBROOT");

    //should exists both or not
    assert((JBRAND && JBROOT) || (!JBRAND && !JBROOT));

    if(JBRAND && JBROOT) {
        __roothideinit_JBRAND = strdup(JBRAND);
        __roothideinit_JBROOT = strdup(JBROOT);
    } else {
        assert(bootstrap() == 0);
    }

    do { // only for jb process because some system process may crash when chdir
        
        char executablePath[PATH_MAX]={0};
        uint32_t bufsize=sizeof(executablePath);
        if(_NSGetExecutablePath(executablePath, &bufsize) != 0)
            break;
        
        char realexepath[PATH_MAX];
        if(!realpath(executablePath, realexepath))
            break;
            
        char realjbroot[PATH_MAX];
        if(!realpath(JBROOT, realjbroot))
            break;
        
        if(realjbroot[strlen(realjbroot)] != '/')
            strcat(realjbroot, "/");
        
        if(strncmp(realexepath, realjbroot, strlen(realjbroot)) != 0)
            break;

        //for jailbroken binaries
        redirectNSHomeDir(JBROOT);
    
        pid_t ppid = __getppid();
        assert(ppid > 0);
        if(ppid != 1)
            break;
        
        char pwd[PATH_MAX];
        if(getcwd(pwd, sizeof(pwd)) == NULL)
            break;
        if(strcmp(pwd, "/") != 0)
            break;
    
        assert(chdir(JBROOT)==0);
        
    } while(0);
}