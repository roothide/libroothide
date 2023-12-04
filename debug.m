#include <Foundation/Foundation.h>


char* getbacktrace()
{
    NSArray *csss = [NSThread callStackSymbols];
    return strdup([NSString stringWithFormat:@"%@", csss].UTF8String);
}
