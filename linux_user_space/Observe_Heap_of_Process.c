#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <unistd.h>

int main()
{
        char *ptr=NULL;


        pid_t pid= getpid();
        printf("Pid : %d\n",pid);

        /* Creating Memmory Leaks to Know the Heap segement Increase in Process address space in PCB 
         *
         *  cat  /proc/pid/maps/
         *
         * */

        while (1)
        {

        ptr=malloc(100);
        for(int i=0;i<100;i++)
                ptr[i]='A';

//usleep(10);
        }

}
~  
