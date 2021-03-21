#include<stdio.h>

int main()
{
	int i = 5;
	printf(" %d  toggle clear  %d\n",i, toggle_and_set(i));
}
int toggle_and_set(int num)
{
	int bits = (sizeof(num) * 8 ) -1;
	int i = 0;
	for(i=0;i<bits;i++)
	{
		( i  % 2 ) == 0 ? (num &=~(1 << i)):(num ^= (1 << i));
	}
	print_bits(num);
	return num;
}

int print_bits(int num )
{
	int bits = (sizeof(num) * 8 ) -1;
	int  i = 0;
	
	for (i= bits;i>=0;i--)
	   (num & (1 << i) ) == 0 ?printf("0"):printf("1");

	printf("\n");
}
