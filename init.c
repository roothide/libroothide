#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
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

static void __attribute__((__constructor__)) _roothide_init()
{    
    const char* JBRAND = getenv("JBRAND");
    const char* JBROOT = getenv("JBROOT");
    
    assert(JBRAND != NULL && JBROOT != NULL);
    
    __roothideinit_JBRAND = strdup(JBRAND);
    __roothideinit_JBROOT = strdup(JBROOT);
    
    do { // only for jb process because some system process may crash when chdir
        pid_t ppid = __getppid();
        assert(ppid > 0);
        if(ppid != 1)
            break;
        
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
        
        char pwd[PATH_MAX];
        if(getcwd(pwd, sizeof(pwd)) == NULL)
            break;
        if(strcmp(pwd, "/") != 0)
            break;
    
        assert(chdir(JBROOT)==0);
        
    } while(0);
}