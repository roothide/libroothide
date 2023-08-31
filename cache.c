#include "cache.h"
#include <stdlib.h>
#include <string.h>

const char* cache_path(struct cache_path* cache, const char* path)
{
    if(!cache->array) {
        static pthread_mutex_t initlock=PTHREAD_MUTEX_INITIALIZER;
        pthread_mutex_lock(&initlock);
        if(!cache->array) {
            pthread_mutex_init(&cache->lock, NULL);
            cache->array = malloc(sizeof(cache->array[0])*CACHE_INIT_SIZE);
            cache->size = CACHE_INIT_SIZE;
            cache->used = 0;
        }
        pthread_mutex_unlock(&initlock);
    }
    
    pthread_mutex_lock(&cache->lock);
    
     const char* cachedpath = NULL;
     
     for(size_t i=0; i<cache->used; i++)
     {
         if(strcmp(path, cache->array[i])==0) {
             cachedpath = cache->array[i];
             break;
         }
     }
     
    if(!cachedpath)
    {
        cachedpath = strdup(path);
        
        cache->array[cache->used] = cachedpath;
        cache->used++;
    }

    if(cache->used==cache->size)
    {
        const char** new_cache_array = malloc(sizeof(cache->array[0])*cache->size*2);
        
        memcpy(new_cache_array, cache->array, sizeof(cache->array[0])*cache->size);
        
        free(cache->array);
        
        cache->size = cache->size*2;
        cache->used = cache->used;
        cache->array = new_cache_array;
    }
    
    pthread_mutex_unlock(&cache->lock);

    return cachedpath;
}
