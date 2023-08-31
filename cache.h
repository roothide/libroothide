
#include <pthread.h>

#define CACHE_INIT_SIZE 100

struct cache_path {
    pthread_mutex_t lock;
    const char** array;
    size_t used;
    size_t size;
};

const char* cache_path(struct cache_path* cache, const char* path);

