// TODO: usage, daemon, log, sse, www, valgrind, load test, MAX of variable length
// headers/*{{{*/
// socket
#include <sys/socket.h>     
// epoll interface
#include <sys/epoll.h>      
// struct sockaddr_in
#include <netinet/in.h>     
// IP addr convertion
#include <arpa/inet.h>      
// File descriptor controller
#include <fcntl.h>
// sysconf etc..
#include <unistd.h>
#include <stdio.h>
// threads
// #include <pthread.h> //already included in loop.h
// string functions
#include <string.h>
// dir
#include <dirent.h>
// malloc(), free()
#include <stdlib.h>
// errors
#include <errno.h>
// args for errexit
#include <stdarg.h>
//
#include <sys/stat.h>
//
#include <libgen.h>
// signals
#include <signal.h>
// loopy definitions
#include "loop.h"
//loopy http
#include "http.h"
// loopy server sent events
#include "sse.h"
// liblfds
//#include liblfds710.h
// booleans
//#include <stdbool.h>/*}}}*/

// macros  MAX should scale to more clients in the future/*{{{*/
#define MAX 20
#define PAGESIZE 4096

// thread status
#define CHILD      (1 << 0) 
#define RUNNING    (1 << 1)
#define WAITING    (1 << 2)
#define TERMINATED (1 << 3)/*}}}*/


// replace with lfds queue
struct Queue
{
    epoll_data_t data;
    struct Queue *next;
};

// network
int listenfd;
socklen_t length;
struct sockaddr_in clientaddr;
struct sockaddr_in serveraddr;

// threads
__thread unsigned indx;

// epoll
int                   epfd;
struct epoll_event    ev;
struct epoll_event    events[MAX];
//epoll_data_t *transport;

void die(void)
{/*{{{*/
//    unsigned n;
    puts("\nLoopy is terminating..\n");
    sleep(4);
    // reversed loop
//    for(n = id; n-- > id;)
//	printf("TIDs: %u", (unsigned) tid[id]);
}/*}}}*/

// terminate program gracefully
void term(int signum)
{/*{{{*/
   puts("\nHandling exit request\n");
}/*}}}*/

int errexit(const char *format, ...)
{/*{{{*/
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(1);
}/*}}}*/

// still need to get host + virtualhost info
void conf(struct sockaddr_in *serveraddress, char *path)
{/*{{{*/
    DIR *dir;
    char *ip;
    char *port;
    char *protocol; // unused
    static struct dirent entry;
    static struct dirent *pentry;
    static struct stat file_info;
    char entrypath[1024];

//    if((MAXBYTE = getpagesize()) < 0)
//	errexit("Failed to get page size Error:%s\n", strerror(errno));
    if(!(dir = opendir(dirname(path))))
	errexit("s%\n", strerror(errno));
    while((readdir_r(dir, &entry, &pentry) == 0) && pentry != NULL)
    {
	sprintf(entrypath,"%s/%s", path, entry.d_name);
	if(lstat(entrypath, &file_info) < 0)
	    errexit("lstat failed: %s\n", strerror(errno));
	if(!S_ISDIR(file_info.st_mode))
	    continue;
	if(strcmp(entry.d_name, "..") == 0 || 
	   strcmp(entry.d_name,  ".") == 0)
	    continue;
	ip = strdup(entry.d_name);
	ip = strsep(&ip, ":");
	if(inet_pton(AF_INET, ip, &(*(serveraddress)).sin_addr) != 1)
	    errexit("%s\n", strerror(errno));
	port = strdup(entry.d_name);
	port += (strlen(ip) + 1);
	port = strsep(&port, "_");
	if(!(serveraddress->sin_port = htons(atoi(port))))
	    errexit("%s\n", strerror(errno));
	protocol = entry.d_name + strlen(port) + strlen(ip) + 2;
	printf("IP: %s\nPort: %s\nProtocol: %s\n", ip, port, protocol);
	return;
    }
}/*}}}*/

static void set_nonblocking(int sock)
{/*{{{*/
    int flags;
    if((flags = fcntl(sock, F_GETFL)) < 0)
	errexit("Failed to retrieve flags\n");
    flags |= O_NONBLOCK;
    if(fcntl(sock, F_SETFL, flags) < 0)
	errexit("Failed to set O_NONBLOCK flag\n");
}/*}}}*/

//fwd declaration of worker func
void *wrkr(void *t);

//depends on structs Member + Threads and global threads 
int make_threads(Threads *threads, void *(*worker)(void *))
{/*{{{*/
    unsigned i = 0;

    printf("Spun Threads: %u\n", (unsigned) threads->count);
    // set main thread at index 0
    threads->thread[i].tid = pthread_self();
    //  loop accounts for main thread
    for(i = 1; i < threads->count; i++)
    {
	if(pthread_create(&threads->thread[i].tid, NULL, worker, threads) < 0)
	    errexit("Failed to create %d threads\n", threads->count);
	sleep(1);
    }
    return 0;
}/*}}}*/

int main(int argc, char **argv)
{/*{{{*/
    unsigned i, cores;
//  int nfds;
    static Threads threads;
    // Handle signals
//    struct sigaction action;
//    memset(&action, 0, sizeof (struct sigaction));
//    action.sa_handler = term;
//    sigaction(SIGINT, &action, NULL);
    // Set exit function
    if((atexit(die)) != 0)
	errexit("atexit() failed\n");

    //server conf
    conf(&serveraddr, argv[0]);
    serveraddr.sin_family = AF_INET;

    // socket
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	errexit("failed to create socket");
    set_nonblocking(listenfd);
    
    // bind
    if((bind(listenfd, (struct sockaddr *)
		    &serveraddr, sizeof serveraddr)) < 0)
	errexit("bind failed Error:%s\n", strerror(errno));
    
    // listen
    if((listen(listenfd, MAX)) < 0)
	errexit("listen failed");

    // Epoll
    epfd = epoll_create1(0);
    ev.data.fd = listenfd;
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT /*| EPOLLEXCLUSIVE*/;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);

    //Calculate threads to spin
    if((cores = sysconf(_SC_NPROCESSORS_ONLN)) > 1)
    {
	threads.count = cores - 1;
    }
    else
	threads.count = cores;
    printf("Cores: %d\n", cores);
    if((threads.thread = calloc(cores, sizeof (Member))) < 0)
	errexit("Calloc failed because: %s\n", strerror(errno));
    make_threads(&threads, wrkr);

    //    for(i = threads.count; i-- > 0;) //reversed order
    for(i = 0; i < threads.count; i++)
    {
	printf("Thread: %d Id: %u\n", i, (unsigned) threads.thread[i].tid);
	//	threads.thread[i].status | RUNNING;
    }
    pthread_join(threads.thread[1].tid, NULL);
    pthread_join(threads.thread[2].tid, NULL);

    printf("\x1B[36m\n");
//    threads.thread[1].tstat ^= RUNNING;
//    threads.thread[2].tstat ^= RUNNING;
    free(threads.thread);
    puts("Main returns");
    pthread_exit(NULL);
}/*}}}*/

void *wrkr(void *tpool)
{/*{{{*/
    int i;
    indx = 0;
    Threads *tp = tpool;

    // allocate memory for read / write buffer

    while (tp->thread[indx].tid != pthread_self()) 
	indx++;
//    tp->thread[indx].tstat |= RUNNING;
//    pthread_detach(pthread_self());

    while(/*tp->thread[indx].tstat & RUNNING*/1)
    {
	printf("\x1B[3%dm...\n", indx);
	tp->thread[indx].nfds = epoll_wait(epfd, events, 10, -1);
	for(i = 0; i < tp->thread[indx].nfds; ++i)
	{
	    if(events[0].data.fd == listenfd)
	    {
		printf("Event (thread)\n");
		if((tp->thread[indx].connfd = accept(listenfd,
		   (struct sockaddr*) &clientaddr, &length)) < 0)
		    //		errexit("accept failed\n");
		    printf("Accept:%d Error: %s\n", tp->thread[indx].connfd, 
			    strerror(errno));
		set_nonblocking(tp->thread[indx].connfd);
		ev.data.fd = tp->thread[indx].connfd;
		ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT /*| EPOLLEXCLUSIVE*/;
		epoll_ctl(epfd, EPOLL_CTL_ADD, tp->thread[indx].connfd, &ev);
		ev.data.fd = listenfd;
		ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT /*| EPOLLEXCLUSIVE*/;
		epoll_ctl(epfd, EPOLL_CTL_MOD, listenfd, &ev);
	    }
	    else
	    {
		printf("Thread: %u executing\n", (unsigned) pthread_self());
		//rearm epoll with listening socket
	    
		   if((tp->thread[indx].read = 
		      recv(tp->thread[indx].connfd,
		      tp->thread[indx].rbuffer, 1024, 0)) < 0)
		       errexit("recvd failed");
		   if (errno == ECONNRESET) //failed to receive
		       close(tp->thread[indx].connfd);
		   else if (tp->thread[indx].read == 0) //client closed conn
		       close(tp->thread[indx].connfd);
		   else
		   {
		    // use modules here.. 
		    http_parse(&(tp->thread[indx]));
		    //best to cast to unsigned long %lu, zd for size_t
		    printf("%u\n", tp->thread[indx].read);
	//	    [(tp->thread[indx].bufferSize)+1] = '\0';
	//	    if(tp->thread[indx].bufferSize == 0)
	//		    printf("zero bytes received\n");
	//	    else
	//	    printf("Request: %s\n", tp->thread[indx].buffer);
		    fwrite(tp->thread[indx].rbuffer, 
			   tp->thread[indx].read, 1, stdout);
		    // Date
		    // Content-Type
		    // Server
//		    char *end;
		    char *reply = "HTTP/1.1 200 OK\n :) :)\n";
		    stpcpy(tp->thread[indx].wbuffer, reply);
//		    strcat(end, ":)");
		    send(tp->thread[indx].connfd,
			 tp->thread[indx].wbuffer,
			 strlen(reply), 0);
		    close(tp->thread[indx].connfd);
	//	    switch(protocol)
	//	    {
	//		case SSE :
	//		    w = handle_sse(data);
	//		    break;
	//	    ,
	//		default :
	//		    w = handle_html(data);
	//         }
	          }
	    }
    printf("\x1B[0m");
	}
    }
    printf("\x1B[0m");
    pthread_exit(NULL);
}/*}}}*/
