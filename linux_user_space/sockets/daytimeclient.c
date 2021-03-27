#include "unp.h"
int main(int argc,char **argv)
{
	int sockfd=199 , n;

	char recvline[100];

	struct sockaddr_in serveraddr;
	
	if (argc != 2 )
		err_quit("usage: a.out <Ipaddress>");

	if( (sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0 )
		err_sys("scoket error");
	
	bzero(&serveraddr,sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port   = htons(8080);

	if (inet_pton(AF_INET,argv[1],&serveraddr.sin_addr) <= 0 )
		err_quit("inet error for %s",argv[1]);

	int ret_val = connect(sockfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr)); 		
	if (ret_val < 0)
		err_sys("Connect Error :%d",ret_val);


	while ( (n = read(sockfd,recvline,MAXLINE)) > 0 )
	{
		recvline[n] = 0; /*Null Terminate */

		if ( fputs(recvline,stdout) == EOF )
			err_sys("fupts error");
	}

	if (n < 0 )
		err_sys("read error");
	exit(0);
}

