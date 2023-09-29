#include <Foundation/Foundation.h>


char* backtrace()
{
    NSArray *csss = [NSThread callStackSymbols];
    return strdup([NSString stringWithFormat:@"%@", csss].UTF8String);
}
