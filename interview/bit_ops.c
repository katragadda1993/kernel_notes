
#include<stdio.h>

void set_bit(int *n,int pos)
{
	*n |= 1<<pos;
	printf("%s: n=%x\n",__func__,*n);
}
void clear_bit(int *n,int pos)
{
	*n &= ~(1<<pos);
	printf("%s: n=%x\n",__func__,*n);
}

/*
1 ^ 1 = 0
0 ^ 1 = 1
*/
void toggle_bit(int *n,int pos)
{
	*n ^= (1<<pos);
	printf("%s: n=%x\n",__func__,*n);
}

// Check First Bit set the given Num is Odd
void even_odd(int n)
{
	if ( n & 1 )
		printf("Odd : %d\n",n);
	else
		printf("Even : %d\n",n);
}

void Opposite_Sign(int x,int y)
{
	int op = (( x ^ y) < 0);
	if(op)
		printf("opposite sign\n");
	else
		printf("don't have opposite sign\n");
}


void swap(int x , int y)
{
	printf("x: %d y: %d \n",x,y);
	x = x ^ y;
	y = x ^ y;
	x = x ^ y;
	printf("x: %d y: %d \n",x,y);
}

void unset_rightmost_bit_set(int n)
{
	n = n & (n -1);
	printf("%s: n=%x\n",__func__,n);
}

void is_powerof_2(int n)
{
	n = n & (n -1);
	if(!n)
		printf("Power of 2\n");
	
}

int ff_s(int n)
{
	if ( n & 1 )
		return 1;
	/* unset right most set bit num and xor with number itself */
	n = n ^ ( n & ( n-1 ));
	
	int pos = 0;
	
	while(n) {
		n = n >> 1;
		pos++;
	}
	printf("ffs: %d\n",pos);
	return pos;
}

int count_setbit(int n)
{
	int count = 0 ;
	while(n)
	{	
		n = n & (n-1);
		count++;
	}
	printf("%s count:%d\n",__func__,count);
	return count;
}

unsigned int swapBits( unsigned int number, unsigned int i, unsigned int j ) 
{
  number ^= (1 << i);
  number ^= (1 << j);
  
  return number;
}

unsigned int swapOddEvenBits( unsigned int number ) 
{
  return ( ( number & 0xAAAAAAAA ) >> 1 ) | ( ( number & 0x55555555 ) << 1 );
}

unsigned int rangeToggle(unsigned int num, unsigned int i , unsigned int j)
{
  return num ^ ((1<<(j-i)+1)-1)<<i;
}

#define INT_BITS 32
#define ROTATE_LEFT(pos,data) (( data << pos) | ( data >> INT_BITS - pos))
#define ROTATE_RIGHT(pos,data) (( data >> pos) | ( data << INT_BITS - pos))

#define SWAP_NIBBLE(data) ((data & 0x0F ) << 4 | (data & 0xF0 ) >> 4)
#define GET_BITS(data, pos) ((data & ( 1 << pos)) >> pos)


int main()
{
}


