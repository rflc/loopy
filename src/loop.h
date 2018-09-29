// Exposes main definitions to modules
#ifndef LOOP_H
#define LOOP_H
// pthreads
#include <pthread.h>
#define PAGESIZE 4096
#define CACHE    32768
#define NBLOCKS  64
#define BLOCK    512
#define LINE     64

// 32 byte structure
typedef struct Member
{
    pthread_t        tid;   //8-15  (8)
    unsigned        read;   //16-19 (4)
    unsigned     written;
    int             nfds;   //20-23 (4)
    int               fd;   //24-27 (4)
    int           connfd;   //28-31 (4)
    char        rbuffer[1024];   //0-7   (8) //1024
    char        wbuffer[1024];
//    int      tstat : 1; 36-..
//    int      hstat : 1;
} Member;

// thread poll
typedef struct Threads
{
    unsigned count;
    Member *thread;
} Threads;

#endif
