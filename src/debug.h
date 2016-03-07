#ifndef __DEBUG_H
#define __DEBUG_H


#define ILOG(fmt, ...) printf("INFO: <%s> " fmt "\n",\
        __FUNCTION__, ##__VA_ARGS__)
#define WLOG(fmt, ...) printf("WARNING: <%s> " fmt "\n",\
        __FUNCTION__, ##__VA_ARGS__)
#define ELOG(fmt, ...) printf("ERROR: <%s> " fmt "\n",\
        __FUNCTION__, ##__VA_ARGS__)

#if DEBUG
#define DLOG(fmt, ...) printf("DEBUG: <%s> " fmt "\n",\
        __FUNCTION__, ##__VA_ARGS__)
#else
#define DLOG(fmt, ...)
#endif

#endif
