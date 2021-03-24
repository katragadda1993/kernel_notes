
/*int main()
{
	int num = 0xAABBCCDA;
	printf("%x\n",num);
	
	num = ( ( num & 0xffff0000 ) >> 16 )| ( ( num & 0x0000ffff )  << 16 );
	printf("%x\n",num);


}
*/

int main()
{
	char buff[]="AAAABBBBCCCCCAAAA";

	char o_buff[128]="";

	memset(o_buff,0x0,sizeof(o_buff));

	int len = strlen(buff)-1;

	int i =0,j =0; 
	
	int count=0;

	for(i=0;i<len;i++)
	{
		if (buff[i]==0x0)
			continue;
		
		for (j=i+1;j<len;j++)
		{
			if(buff[i] == buff[j])
				buff[j]=0x0;
		}
		o_buff[count++]=buff[i];
	}
	printf("%s\n",o_buff);
}
