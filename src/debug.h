#ifndef __DEBUG_H
#define __DEBUG_H

#define DEBUG 1
#if DEBUG
#define DLOG(fmt, ...) printf("INFO: <%s> " fmt, __FUNCTION__, ##__VA_ARGS__)
#else
#define DLOG(fmt, ...)
#endif

#endif
