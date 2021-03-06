#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <sys/ioctl.h>

static int gl11_gsmatcmd_response(char *Cmd, char *Resp, char *Ansr[], int Tout);

static int gsm_modem_pwroff_sub (void);
static int gps_pwroff_sub (void);
static int gsm_modemclose(void);
static int gsm_modemopen(char *Pname, int Brate);

int gl11_gsm_usb_pwr_cntrl(int);
int gl11_gsmpwrctrl(int);
int gl11_simselect(int);
int gl11_modem_reset(void);

int gl11_gsm_select(int sel,int baudrate);
int gl11_gsm_pwroff(void);

int gl11_gps_pwron(void);
int gl11_gps_pwroff(void);

static int gsm_mfd=-1, gsm_brate=115200;//gsm_mfd:Its for GSM Modem Open File Discriptor and gsm_brate:Its to Store The GSM Modem IO Bauderate. 

int lock_modem (int time_out)
{
	FILE *fp;
	int timeout = time_out * 10;
	char *file="/tmp/gsm_gps";

	while (timeout--)
	{
		fp=fopen(file, "r");

		if (fp==NULL)
			break;
		else
			fclose(fp);
		usleep(100000);
		if ( timeout %10 ==0)
			fprintf(stdout,"Waiting for lock %s\n",file);
	}

	if ( !timeout )
	{
		fprintf(stderr,"Unable to Acquire lock\n");
		return -1;
	}

	fp = fopen(file, "w");
	fclose(fp);
	return 0;
}

void unlock_modem (void)
{
	remove("/tmp/gsm_gps");
}

int gsm_modem_open()
{
	int ret=0;
	ret= gsm_modemopen("/dev/gsm", 115200);
	return ret;
}
int gsm_modem_close()
{
	int ret=0;
	ret= gsm_modemclose();
	return ret;

}

int gps_open()
{
	int ret=0;
	ret= gsm_modemopen("/dev/gps", 115200);
	return ret;
}
int gps_close()
{
	int ret=0;
	ret=gsm_modemclose();
	return ret;
}

//Its for Set a Baudrate to Particular BR:Baudrate.
static void gsm_setbaudrate(int BR)
{
	struct termios newtio;

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag     = CS8 | CREAD;
	newtio.c_iflag     = IGNPAR;
	newtio.c_oflag     = 0;
	newtio.c_lflag     = 0;
	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN]  = 0;

	switch(BR)
	{
		case 300:    BR=B300;   break;
		case 1200:   BR=B1200;  break;
		case 2400:   BR=B2400;  break;
		case 9600:   BR=B9600;  break;
		case 19200:  BR=B19200; break;
		case 38400:  BR=B38400; break;
		case 57600:  BR=B57600; break;
		case 115200: BR=B115200; break;
		case 230400: BR=B230400; break;
		case 460800: BR=B460800; break;
		case 921600: BR=B921600; break;
		case 1152000:BR=B1152000;break;
		case 3000000:BR=B3000000;break;
		case 3500000:BR=B3500000;break;
		case 4000000:BR=B4000000;break;
		default: BR=B115200;
	}
	cfsetispeed(&newtio, BR);
	cfsetospeed(&newtio, BR);
	tcsetattr(gsm_mfd, TCSANOW, &newtio);
}

//Its for Close the dev Ports fds Note: If cfd=0 Close All fd's - Otherwise Close Only that One.
static int gsm_modemclose(void)
{
	short ssk=0;

	ssk = close(gsm_mfd);
	gsm_mfd=-1;
	usleep(10000);
	return ssk;
}

//Its for Open The Dev Ports, Portname:Port Name like /dev/gsm and Brate:Baudrate like 115200.
static int gsm_modemopen(char *Pname, int Brate)
{
	if(gsm_mfd >0)
		gsm_modemclose();

	gsm_mfd = open(Pname, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if(gsm_mfd <0)
	{
		fprintf(stderr,"LIB#Cannot the Serial Port with name %s\n",Pname);
		return -1;
	}
	gsm_setbaudrate(Brate);	
	return gsm_mfd;
}

//Its for Response of Particular AT Commands, Cmd:AT_Command, Resp:Response Buffer Ansr:Answer of The Response and Tout:Timeout Value.
int gsm_atcmd_response(char *Cmd, char *Resp, char *Ansr[], int Tout)
{
	if((gsm_mfd = open("/dev/gsm", O_RDWR | O_NOCTTY | O_NONBLOCK)) <0){
		fprintf(stderr,"LIB#1./dev/gsm Port Open Error\n");
		return -4;
	}
	close(gsm_mfd);
	gsm_mfd =-1;

	//char *Ansr[] ={"OK", "ERROR", "NO DIALTONE", "NO CARRIER", NULL};
	struct timeval tv;
	fd_set rfds;
	tv.tv_sec=Tout;
	tv.tv_usec=0;
	int ssk=0, i=0, Abytes=0, Tbytes=0, Rbytes=0;

	if(gsm_modemopen("/dev/gsm", gsm_brate) <0)
		return -1;

	FD_ZERO(&rfds);
	FD_SET(gsm_mfd, &rfds);
	tcflush(gsm_mfd, TCIOFLUSH);
	memset(Resp, 0, sizeof(Resp));

	if(write(gsm_mfd, Cmd, strlen(Cmd)) <0){
		fprintf(stderr,"LIB#Writing Command(%.*s) to The Modem Failed\n",strlen(Cmd)-2,Cmd);
		gsm_modemclose();
		return -1;
	}

	do{
		ssk = select(gsm_mfd+1, &rfds, NULL, NULL, &tv);
		if(ssk >0){
			ioctl(gsm_mfd, FIONREAD, &Abytes);
			if(Abytes >0)
				Rbytes = read(gsm_mfd, (Resp+Tbytes), Abytes);
			if(Rbytes >0){
				Tbytes +=Rbytes;
				Resp[Tbytes] ='\0';
			}
		}
		for(i=0; Ansr[i]!=NULL; i++){
			if(strstr(Resp, Ansr[i])){
				//fprintf(stdout,"LIB#Response[%d]:%s\n",i,Resp);
				gsm_modemclose();
				return i;
			}
		}
		usleep(15000);
	}while(tv.tv_sec >0);
	//fprintf(stdout,"LIB#Resp of Module(%.*s): TIMEOUT\n",strlen(Cmd)-2,Cmd);
	gsm_modemclose();
	return -2;//Return Value is:-2
}

//Its for Power On The GSM Module and Then Checking AT Command Response.
static int gsm_at(int opt)
{
	int ret=0, count=1;
	char gresp[1024]="";
	char *Ansr[] ={"OK", "ERROR", "NO DIALTONE", "NO CARRIER", NULL};
	char *Ansr1[] ={"READY", "ERROR", NULL};
	char cmd[100]="";

	if(opt ==0)
	{
		//printf("GSM Node Checking-------->\n");

		for(ret=0; ret<20; ret++)
		{
			sprintf(cmd,"udevadm info --name=/dev/ttyUSB%d --attribute-walk | grep 1e0e",ret);
			if(system(cmd) ==0)
			{	
				printf("Module - 4G\n");
				memset(cmd, 0, sizeof(cmd));
				sprintf(cmd,"ln -s /dev/ttyUSB%d /dev/gps &> /dev/null",ret+1);
				system(cmd);

				memset(cmd, 0, sizeof(cmd));
				sprintf(cmd,"ln -s /dev/ttyUSB%d /dev/gsm &> /dev/null",ret+2);
				system(cmd);

				memset(cmd, 0, sizeof(cmd));
				sprintf(cmd,"ln -s /dev/ttyUSB%d /dev/data &> /dev/null",ret+3);
				system(cmd);
				sleep(10);
				count=0;
				break;
			}else	sleep(1);
		}
		if(count ==1)
		{
			fprintf(stderr,"LIB#Ports Not Created\n");
			return -1;
		}
	}
	else if(opt ==1)
	{
		//printf("Ports Already Exists\n");

		gsm_atcmd_response("AT+CFUN=0\r\n", gresp, Ansr, 10);
		sleep(4);
		gsm_atcmd_response("AT+CFUN=1\r\n", gresp, Ansr, 10);
		sleep(4);

	}

start_at:

	memset(gresp, 0, sizeof(gresp));
	if((ret=gsm_atcmd_response("AT\r\n", gresp, Ansr, 10)) !=0)
	{
		if(count++ >4){
			fprintf(stdout,"LIB#AT ERROR\n");
			return ret;
		} else {
			sleep(1);
			goto start_at;
		}
	}
	fprintf(stdout,"LIB#AT_OK\n");
	count =0;
	sleep(2);
start_cfun:

	memset(gresp, 0, sizeof(gresp));
	if((ret=gsm_atcmd_response("AT+CFUN=1\r\n", gresp, Ansr, 10)) !=0)
	{
		if(count++ >4){
			fprintf(stdout,"LIB#AT+CFUN=1 ERROR\n");
			return ret;
		} else {
			sleep(1);
			goto start_cfun;
		}
	}
	fprintf(stdout,"LIB#AT+CFUN=1_OK\n");
	count =0;
	sleep(2);
start_cpin:

	memset(gresp, 0, sizeof(gresp));
	if((ret=gsm_atcmd_response("AT+CPIN?\r\n", gresp, Ansr1, 10)) !=0)
	{
			if(count++ >10){
				fprintf(stdout,"LIB#AT+CPIN? ERROR\n");
				return -3;//Return Value is:-3 for Sim Not Present
			} else{
				sleep(1);
				goto start_cpin;
			}
	}
	fprintf(stdout,"LIB#AT+CPIN?_OK\n");

	sprintf(cmd,"AT+IPR=%d\r\n",gsm_brate);
	if((ret=gsm_atcmd_response(cmd, gresp, Ansr, 10)) ==0)
		fprintf(stdout,"LIB#AT+IPR=%d_OK\n",gsm_brate);
	else
		fprintf(stdout,"LIB#AT+IPR=%d_ERROR\n",gsm_brate);
	return ret;
}

static int gps_at (int opt)
{
	int ret=0, count=1;
	char gresp[1024]="";
	char *Ansr[] ={"OK", "ERROR", "NO DIALTONE", "NO CARRIER", NULL};
	char cmd[100]="";

	if(opt ==0)
	{

		for(ret=0; ret<20; ret++)
		{

			sprintf(cmd,"udevadm info --name=/dev/ttyUSB%d --attribute-walk | grep 1e0e",ret);
			if(system(cmd) ==0)
			{
				memset(cmd, 0, sizeof(cmd));
				sprintf(cmd,"ln -s /dev/ttyUSB%d /dev/gps &> /dev/null",ret+1);
				system(cmd);

				memset(cmd, 0, sizeof(cmd));
				sprintf(cmd,"ln -s /dev/ttyUSB%d /dev/gsm &> /dev/null",ret+2);
				system(cmd);

				memset(cmd, 0, sizeof(cmd));
				sprintf(cmd,"ln -s /dev/ttyUSB%d /dev/data &> /dev/null",ret+3);
				system(cmd);
				sleep(10);
				count=0;
				break;
			}else	sleep(1);
		}
		if(count ==1)
		{
			fprintf(stderr,"LIB#Ports Not Created\n");
			return -1;
		}
	}
start_gpsat:

	memset(gresp, 0, sizeof(gresp));
	if((ret=gsm_atcmd_response("AT\r\n", gresp, Ansr, 10)) !=0)
	{
		if(count++ >4){
			fprintf(stdout,"LIB#AT ERROR\n");
			return ret;
		} else {
			sleep(1);
			goto start_gpsat;
		}
	}
	fprintf(stdout,"LIB#AT_OK\n");
	count =0;

start_gpsatcfun: 

	if ( opt == 0 )
	{
		memset(gresp, 0, sizeof(gresp));
		if((ret=gsm_atcmd_response("AT+CFUN=1\r\n", gresp, Ansr, 10)) !=0)
		{
			if(count++ >4){
				fprintf(stdout,"LIB#AT+CFUN=1 ERROR\n");
				return ret;
			} else {
				sleep(1);
				goto start_gpsatcfun;
			}
		}
		fprintf(stdout,"LIB#AT+CFUN=1_OK\n");
		count =0;
	}

	ret=gl11_gps_pwr_on();

	printf("GPS Antpwron:%d\n",ret);

	ret=gsm_atcmd_response("AT+CGPS?\r\n", gresp, Ansr, 10);
	if(ret==0)
	{
		if(strstr(gresp,"+CGPS: 1,1")!=NULL)
			return 0;			

	}

start_cgps:
	memset(gresp, 0, sizeof(gresp));

	ret=gsm_atcmd_response("AT+CGPS=1\r\n", gresp, Ansr, 10);
	printf("CGPS Response:%s\n",gresp);
	if(ret!=0)
	{
		if(count++ > 4){
			fprintf(stdout,"LIB#AT+CGPS=1 ERROR\n");
			return ret;
		} else {
			sleep(1);
			goto start_cgps;
		}
	}

	fprintf(stdout,"LIB#AT+CGPS=1_OK\n");
	count =0;
	return ret;
}

//Its for Switching The SIM-1 to SIM-2 Slot Using Hardware Reset Pin and AT Command

static int gsm_athselect(int sel, int baudrate)
{

	int ret=0, ret1=0, retry_count=0, i=0;

	if(sel ==1)
	{
		gl11_simselect(1);
		gl11_simselect(1);
	}
	else if(sel ==2)
	{
		gl11_simselect(0);
		gl11_simselect(0);
	}
	printf("SIM Selected\n");


	gsm_mfd = open("/dev/gsm", O_RDWR | O_NOCTTY | O_NONBLOCK);
	if(gsm_mfd < 0)
	{
hw_retry:
		system("rm -f /dev/gsm &> /dev/null");
		system("rm -f /dev/data &> /dev/null");
		ret=gl11_gsm_usb_pwr_cntrl(0);
		ret=gl11_gsm_usb_pwr_cntrl(0);
		usleep(500000);
		
		ret=gl11_gsmpwrctrl(0);
		ret=gl11_gsmpwrctrl(0);
		usleep(500000);

		ret=gl11_gsmpwrctrl(1);
		ret=gl11_gsmpwrctrl(1);
		usleep(500000);
		
		ret=gl11_gsm_usb_pwr_cntrl(1);
		ret=gl11_gsm_usb_pwr_cntrl(1);
		sleep(2);

sw_retry:
		for(i=0; i<32; i++)
		{
			//printf("Checking For Vendor and Product IDs.....\n");
			ret=system("lsusb | grep 1e0e:9001 &> /dev/null");
			if(ret!=0)	
				sleep(1);
			else 
				break;

		}
		if(i>31 && retry_count++ <3)
		{
			if(retry_count ==2)
				goto hw_retry;

			ret=gl11_modem_reset() ;//Hardware Reset Through Software
			if(ret<0)
				goto hw_retry;

			sleep(3);
			goto sw_retry;
		}
		else if(i>31 && retry_count >3)
			return -1;

		return gsm_at(0);//Its for Create a Symbolic links then Check Response
	}
	close(gsm_mfd);
	gsm_mfd =-1;
	sleep(1);
	return gsm_at(1);//Its for Switching to Another Slot then Check Response
}

static int gps_init(int baudrate)
{
	int ret=0, ret1=0, retry_count=0, i=0;
	int val=0;

	//printf("GPS_Init.......\n");

	val=gl11_gps_pwr_on();
	if((gsm_mfd = open("/dev/gsm", O_RDWR | O_NOCTTY | O_NONBLOCK)) <0)
	{
gps_hw_retry:
		system("rm -f /dev/gsm &> /dev/null");
		system("rm -f /dev/data &> /dev/null");

		gl11_gsm_usb_pwr_cntrl(0);
		gl11_gsm_usb_pwr_cntrl(0);
		usleep(500000);
		gl11_gsmpwrctrl(0);
		gl11_gsmpwrctrl(0);
		usleep(500000);

		gl11_gsmpwrctrl(1);
		gl11_gsmpwrctrl(1);
		usleep(500000);
		gl11_gsm_usb_pwr_cntrl(1);
		gl11_gsm_usb_pwr_cntrl(1);
		sleep(2);

gps_sw_retry:
		for(i=0; i<32; i++)
		{
			//printf("Checking For Vendor and Product IDs...\n");
			if(system("lsusb | grep 1e0e:9001 &> /dev/null") !=0)

				sleep(1);
			else break;

		}
		if(i>31 && retry_count++ <3){
			if(retry_count ==2)
				goto gps_hw_retry;
			if(gl11_modem_reset() <0)//Hardware Reset Through Software
				goto gps_hw_retry;
			sleep(3);
			goto gps_sw_retry;
		}
		else if(i>31 && retry_count >3)
			return -1;
		return gps_at(0);//Its for Create a Symbolic links then Check Response
	}
	close(gsm_mfd);
	gsm_mfd =-1;
	sleep(1);

	return gps_at(1);	

}

int gl11_gps_pwron (void)
{
	int ret;
	ret = lock_modem(100);
	if (ret == -1)
		goto gps_out;
	ret = gps_init(115200);
	if (ret == -1)
		goto gps_out_one;
	unlock_modem();
	return 0;
gps_out_one:
	unlock_modem();
gps_out:
	return -1;
}




/****************************************************************
 *		Its Functions For User Level			*
 * *************************************************************/

//Its for The Power of All GSM Module Functionality.
int gl11_gsm_pwroff(void)
{

	lock_modem(50);
	gsm_modemclose();
	gsm_modem_pwroff_sub();
	unlock_modem();
	return 0;
}

int gl11_gps_pwroff(void)
{
	lock_modem(50);
	gsm_modemclose();
	gps_pwroff_sub();
	unlock_modem();
	return 0;
}


//Its for Switching The SIM-1 to SIM-2 Slots.

int gl11_gsm_select(int sel,int baudrate)
{
	int ret;

	gsm_brate = baudrate;

	if(sel<1 || sel>2)
	{
		//fprintf(stderr,"LIB#GSM SIM Select Range:1-2\n");
		return -2;
	}

	fprintf(stdout,"\nLIB#SIM-%d SELECTED\n",sel);

	ret = lock_modem(50);

	if (ret == -1)
	{
		goto gsm_out;
	}

	ret = gsm_athselect( sel , 115200 );

	unlock_modem();

	return ret;

gsm_out:


	return -1;
}

//Its for Response of Particular AT Commands, Cmd:AT_Command, Resp:Response Buffer Ansr:Answer of The Response and Tout:Timeout Value.
static int gl11_gsmatcmd_response(char *Cmd, char *Resp, char *Ansr[], int Tout)
{
	short ssk=0;

	if((gsm_mfd = open("/dev/gsm", O_RDWR | O_NOCTTY | O_NONBLOCK)) <0){
		fprintf(stderr,"LIB#2./dev/gsm Port Open Error\n");
		return -4;
	}
	close(gsm_mfd);
	gsm_mfd =-1;
	return gsm_atcmd_response(Cmd, Resp, Ansr, Tout);
}


static int gps_pwroff_sub(void) 
{
	int ret=0;
	int count=1;
	char gresp[1024]="";

	char *Ansr[] ={"OK", "ERROR", "NO DIALTONE", "NO CARRIER", NULL};

	if(system("ls /dev/gsm &> /dev/null") ==0)
	{
		ret=gsm_atcmd_response("AT+CFUN?\r\n", gresp, Ansr, 5);
		printf("CFUN Response:%s\n",gresp);
		if(strstr(gresp,"+CFUN: 1")!=NULL)
		{
disable_gps:
			printf("CFUN Init\n");
			memset(gresp, 0, sizeof(gresp));
			gl11_gps_pwr_off();
			if((ret=gsm_atcmd_response("AT+CGPS=0\r\n", gresp, Ansr, 10)) !=0)
			{
				if(count++ >4){
					fprintf(stdout,"LIB#AT+CGPS=0 ERROR\n");
					return ret;
				}
				else {
					sleep(1);
					goto disable_gps;
				}
			}

			fprintf(stdout,"LIB#AT+CGPS=0_OK\n");
			count=0;

			return ret;
		}
		else if(strstr(gresp,"+CFUN: 0")!=NULL)
		{
			printf("CFUN Deinit\n");

			ret = system("rm -f /dev/gps /dev/gsm /dev/data &> /dev/null");

			gl11_gps_pwr_off();
			gl11_gsm_usb_pwr_cntrl(0);
			gl11_gsmpwrctrl(0);
			sleep(1);
			return ret;

		}

	}
	return 0;
}

static int gsm_modem_pwroff_sub (void)
{
	int ret=0;
	int count=1;
	char gresp[1024]="";
	char *Ansr[] ={"OK", "ERROR", "NO DIALTONE", "NO CARRIER", NULL};

	if(system("ls /dev/gsm &> /dev/null") ==0)
	{
		ret=gsm_atcmd_response("AT+CGPS?\r\n", gresp, Ansr, 5);
		printf("CGPS Response:%s\n",gresp);
		if(strstr(gresp,"+CGPS: 1,1")!=NULL)
		{
disable_gsm:

			printf("CGPS Enabled\n");
			memset(gresp, 0, sizeof(gresp));
			if((ret=gsm_atcmd_response("AT+CFUN=0\r\n", gresp, Ansr, 10)) !=0)
			{
				if(count++ >4){
					fprintf(stdout,"LIB#AT+CFUN=0 ERROR\n");
					return ret;
				}
				else {
					sleep(1);
					goto disable_gsm;
				}
			}

			fprintf(stdout,"LIB#AT+CFUN=0_OK\n");
			count=0;

			return ret;

		}
		else if(strstr(gresp,"+CGPS: 0,1")!=NULL)
		{
			printf("CGPS Deinit\n");
			ret = system("rm -f /dev/gps /dev/gsm /dev/data &> /dev/null");

			gl11_gps_pwr_off();
			gl11_gsm_usb_pwr_cntrl(0);
			gl11_gsmpwrctrl(0);

			sleep(1);

			return ret;
		}

	}
	return 0;

}


