
#define JB_ROOT_PARENT "/var"
#define JB_ROOT_PREFIX ".jbroot-"
#define JB_RAND_LENGTH  (sizeof(uint64_t)*sizeof(char)*2)

#define ROOTFS_NAME   "rootfs"

#include <sys/syslog.h>
#define SYSLOG(...) {openlog("roothide",LOG_PID,LOG_AUTH);syslog(LOG_DEBUG, __VA_ARGS__);closelog();}

int is_jbroot_name(const char* name);

extern const char* __roothideinit_JBRAND;
extern const char* __roothideinit_JBROOT;

#define EXPORT __attribute__ ((visibility ("default")))

#define JBPATH_LOG(...) {if(getenv("JBPATHLOG")&&atoi(getenv("JBPATHLOG"))) {printf(__VA_ARGS__);fflush(stdout);}}

#define VROOT_LOG(...) {if(getenv("VROOTLOG")&&atoi(getenv("VROOTLOG")))printf(__VA_ARGS__);fflush(stdout);}

