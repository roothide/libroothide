
#define JB_ROOT_PARENT "/var"
#define JB_ROOT_PREFIX ".jbroot-"
#define JB_RAND_LENGTH  (sizeof(uint64_t)*sizeof(char)*2)

#define ROOTFS_PREFIX   "/rootfs"

#define LOG(...)

//#define LOG(...) {char* jbpathlog = getenv("JBROOTLOG"); if((jbpathlog && *jbpathlog)||access("/var/.jbrootlog", F_OK)==0) {printf(__VA_ARGS__);fflush(stdout);}}

#include <sys/syslog.h>
//#define LOG(...) {openlog("roothide",LOG_PID,LOG_AUTH);syslog(LOG_DEBUG, __VA_ARGS__);closelog();}

int is_jbroot_name(const char* name);

#define EXPORT __attribute__ ((visibility ("default")))

