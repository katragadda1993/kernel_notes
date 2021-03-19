#include<stdio.h>
#include<string.h>

int set_bit(int num,int position)
{
	num |= (1 << position);
}

int clear_bit(int num,int position)
{
        num &= ~ (1 << position);
}

int show_bits(int num)
{
	 int bits = (sizeof(num)*8)-1;
	for(bits;bits>=0;bits--)
	{	if ( ( num  & (1 << bits) ) == 0 )
		printf("0");
		else
			printf("1");
	}
	puts("\n");
}

int set_odd_bits(int num)
{
	int bits = (sizeof(num)*8)-1;
	for(bits;bits>=0;bits--)
	{
		if ( (bits % 2) != 0)
			num |= (1 << bits);
	}
	show_bits(num);
}

int set_even_bits(int num)
{
        int bits = (sizeof(num)*8)-1;
        for(bits;bits>=0;bits--)
        {
                if ( (bits % 2) == 0)
                        num |= (1 << bits);
        }
        show_bits(num);
}


int count_set_bits(int num)
{
	
	int count=0;
	int bits = (sizeof(num)*8) -1 ;
        
	for(bits;bits>=0;bits--)
        {       if ( ( num  & (1 << bits) ) )
		count++;
        }
	printf("%d\n",count);
	show_bits(num);
	return count;

}

int reverse(int num)
{
	int bits = (sizeof(num)*8) - 1;
	int rev  = 0;
	show_bits(num);
	for(int i=0;i<bits;i++,bits--)
	{
		if  ( num  & (1 << i ) )
			rev |= (1 << bits );
	}
	show_bits(rev);
}

int main()
{
	reverse(19);
}
