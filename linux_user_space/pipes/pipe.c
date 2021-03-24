#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>


/* Pipe is memory buffer allocated in kernel space   and attached to user process adress space it has read / write end */

/* Max Buffer size of pipe is 64K */

/* Unix having pipe system call to create , here required two file descriptors*/


char gbuf[100]="Hello World";

int main()
{
	int pipe1[2]={0,0};
	int pipe2[2]={0,0};

	char buf[100]="";

	int ret_val = 0;

	ret_val = pipe(pipe1);

	if (ret_val < 0)
	{
	       printf("pipe1 creation failed\n");
	       return -1;
	}

	ret_val = pipe(pipe2);

        if (ret_val < 0)
        {
               printf("pipe2 creation failed\n");
               return -1;
        }


	int pid = fork();

	if (pid == 0)
	{
		read(pipe1[0],buf,sizeof(buf));
		write(pipe2[0],gbuf,sizeof(gbuf));
		printf("Recvied data From Child Process :%s\n",buf);

	}

	else
	{
		read(pipe2[1],buf,sizeof(buf));

		sleep(1);
		printf("Recvied data From main process :%s\n",buf);
		write(pipe1[1],gbuf,sizeof(gbuf));

	}

}

	










