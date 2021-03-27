#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include <netinet/in.h>
#include<unistd.h>
#include<stdlib.h>
#include<arpa/inet.h>

#define MAXLINE 80

#define debug(fmt, args...) {printf("Debug:"fmt"\n",##args);}
#define err_sys(fmt, args...) {printf(fmt"\n",##args);exit(-1);}
#define err_exit(fmt, args...) {printf(fmt"\n",##args);exit(-2);}
#define err_quit(fmt, args...) {printf(fmt"\n",##args);exit(-3);}
