#include<stdio.h>
/*
Enumeration (or enum) is a user defined data type in C. 
It is mainly used to assign names to integral constants, the names make a program easy to read and maintain.

*/
enum KMC{
	KMC_RW=19,
	KMC_W,
	KMC_WR,
};

int main()
{
	printf("%d\n",KMC_W);
}
