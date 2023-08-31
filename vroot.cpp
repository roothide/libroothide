#include <stdio.h>
#include <string>

#include "libroothide.h"

#define VROOT_API_NAME(NAME) vroot_##NAME
#define VROOTAT_API_NAME(NAME) vroot_##NAME

using namespace std;

extern "C" {
void* _ZNSt3__114basic_ifstreamIcNS_11char_traitsIcEEE4openEPKcj(void* thiz, const char* __s, unsigned int mode);
void* _ZNSt3__114basic_ofstreamIcNS_11char_traitsIcEEE4openEPKcj(void* thiz, const char* __s, unsigned int mode);
void* _ZNSt3__114basic_ifstreamIcNS_11char_traitsIcEEE4openERKNS_12basic_stringIcS2_NS_9allocatorIcEEEEj(void* thiz, string* __s, unsigned int mode);
void* _ZNSt3__114basic_ofstreamIcNS_11char_traitsIcEEE4openERKNS_12basic_stringIcS2_NS_9allocatorIcEEEEj(void* thiz, string* __s, unsigned int mode);

void* VROOT_API_NAME(_ZNSt3__114basic_ifstreamIcNS_11char_traitsIcEEE4openERKNS_12basic_stringIcS2_NS_9allocatorIcEEEEj)(void* thiz, string* __s, unsigned int mode)
{
    char* newpath = (char*)jbroot_alloc(__s->c_str());
    void* ret = _ZNSt3__114basic_ifstreamIcNS_11char_traitsIcEEE4openEPKcj(thiz,newpath,mode);
    if(newpath) free((void*)newpath);
    return ret;
}

void* VROOT_API_NAME(_ZNSt3__114basic_ofstreamIcNS_11char_traitsIcEEE4openERKNS_12basic_stringIcS2_NS_9allocatorIcEEEEj)(void* thiz, string* __s, unsigned int mode)
{
    char* newpath = (char*)jbroot_alloc(__s->c_str());
    void* ret = _ZNSt3__114basic_ofstreamIcNS_11char_traitsIcEEE4openEPKcj(thiz,newpath,mode);
    if(newpath) free((void*)newpath);
    return ret;
}

}
