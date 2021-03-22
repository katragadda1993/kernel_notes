#include<stdio.h>
/*int main()
{
    // pointer to constant
    int a=100;  
    int b=200;  
    const int* ptr;  
    ptr=&a;  
    ptr=&b;
    printf("Value of ptr is :%d",*ptr);  
    return 0;  
}
*/

int main()
{
	// constant pointer
	int a = 10;
	int *const ptr=&a;
	*ptr=100;
	// Here we can't assign address in pointer variabe its readonly 
	
}
