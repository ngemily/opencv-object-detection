#ifndef __DEBUG_H
#define __DEBUG_H

#define DEBUG 1


#if DEBUG

#define DLOG(fmt, ...) printf("INFO: <%s> " fmt "\n",\
        __FUNCTION__, ##__VA_ARGS__)
#define WLOG(fmt, ...) printf("WARNING: <%s> " fmt "\n",\
        __FUNCTION__, ##__VA_ARGS__)
#define ELOG(fmt, ...) printf("ERROR: <%s> " fmt "\n",\
        __FUNCTION__, ##__VA_ARGS__)

#else

#define DLOG(fmt, ...)
#define WLOG(fmt, ...)
#define ELOG(fmt, ...)

#endif

#endif
