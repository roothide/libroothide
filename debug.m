// //fix NSFoundation headers for old llvm/clang
//
// #if !__has_feature(format_arg)
// #define NS_FORMAT_ARGUMENT(A)
// #endif

// #if !__has_feature(_Nullable_result)
// #define _Nullable_result
// #endif

// #include <Foundation/Foundation.h>

#include <string.h>
#include <Foundation/NSString.h>
#include <Foundation/NSThread.h>

char* getbacktrace()
{
    NSArray *csss = [NSThread callStackSymbols];
    return strdup([NSString stringWithFormat:@"%@", csss].UTF8String);
}
