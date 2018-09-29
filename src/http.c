#include <stdio.h> // remove
#include <time.h>
#include <string.h>
#include "loop.h"
struct
{
    char *ext;
    char *filetype;
} extensions [] = 
{
    {"gif",  "image/gif" },
    {"jpg",  "image/jpg" },
    {"jpeg", "image/jpeg"},
    {"png",  "image/png" },
    {"ico",  "image/ico" },
    {"zip",  "image/zip" },
    {"gz",   "image/gz"  },
    {"tar",  "image/tar" },
    {"htm",  "text/html" },
    {"html", "text/html" },
    {0,0}
};

void http_parse(Member *t)
{
    int i;
    int x;
//    char *token;
    char *str;
//    char *array;
//    char **words = NULL;
    char word[20]; //replace with buffer for TLB hit?
    int  lcount;
    int  wcount;
    int  len1;
    char *method;
    char *path;
//    char *file;
    t->rbuffer[t->read+1] = '\0';
    str = strdup(t->rbuffer);
    method = strsep(&str, " ");
    len1 = strlen(method);
    str = strdup(t->rbuffer+len1+1);
    path = strsep(&str, " ");
    path++;
    len1 = strlen(path);
    printf("PATH: %s\n", path);
    str = strdup(path);
    printf("STR: %s\n", str);
    wcount = 0;
    lcount = 0;
//    word = '\0';
    for(i=0; path[i] != '\0'; i++)
    {
	if(path[i] != '/')
	{
	    lcount++;
	}
	else
	{
	    memcpy(word, path, lcount);
	    
            word[lcount] = 0;
	    printf("/");
	    for(x = 0; x < lcount; x++)
	    {
		//words[wcount] = word+'\0';
	    }
	    wcount++;
//	    printf("%s\n", words[wcount]);
	}
    }
    printf("\n");
/*    for(len1 > 0)
    {
	str = strsep(&str, "/");
	printf("%s\n", str);
	len2 = strlen(str);
	str = strdup(path+len2+1);
    }*/
    printf("%s\n", method);
}

void http_response(void /*hstat*/)
{
    //adjust to possible max value
    char reply[1000];
/*    int len; */
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
	
    memcpy(reply, "HTTP/1.1 200 OK\n"
	          "content-type: text/html; charset=UTF-8\n" 
	          "Date: ",61);
/*    len =*/ strftime(reply+61, sizeof reply, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    printf("%s\n", reply);
}
