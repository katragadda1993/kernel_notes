#include<stdio.h>

void print_bits(int num)
{
	int  max_bits = (sizeof(num)  *8  )-1;
	for (;max_bits>=0;max_bits--)
		 (num & (1<< max_bits) ) == 0 ? printf("0"):printf("1");
	printf("\n");
}


int get_niblle_reverse(int num)
{
	
	int tmp=0;
	int j=0;

	for(int i=3;i>=0;i--,j++)
	   {
		   if ( (num & (1 << i ))  )
		   {
			   tmp |=( 1 << j );
		   }

	   }		   
	return tmp; 
}

int reverse_niblles_num(int num)
{
	int tmp = 0;
	int rev = 0;
	
	for(int i=0;i<32;i=i+4)
	{
		tmp = ( num >> i ) & 0xf;
		tmp = get_niblle_reverse(tmp);
		rev |= (tmp << i);
		tmp=0;
	}
	print_bits(rev);

	return rev;
}

int main()
{
	int num =33;
	
	print_bits(num);

	printf("Reverse all Nibles %d\n",reverse_niblles_num(num));
}
