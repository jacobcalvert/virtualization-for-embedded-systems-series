/**
 *
 */
#include <stdio.h>  
#include <unistd.h>  
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>
#include <termios.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <signal.h>
#include <errno.h>
typedef struct
{
	int tty_fd;
	int sock_fd;
	pthread_t sock_thread;
	struct sockaddr_in client;
	int clilen;
	volatile int run;


}config_settings_t;


static char *DEFAULT_DEVICE = "/dev/ttyACM0";

void modem_log(char *fmt, ...);
int modem_setup(config_settings_t *cfg, char *device, int port);

void *modem_udp_server_task(void *arg);
void *modem_tty_server_task(void *arg);

void exit_handler(int sig);

config_settings_t settings;


int main(int argc, char **argv)
{

	char *device = NULL;
	int port = 2020;
	char opt;
	
	while((opt = getopt(argc, argv, "d:p:")) != -1)  
	{  
		switch(opt)  
		{  
			case 'd':
			{
				device = optarg;
				modem_log("Device selected is '%s'", device);
				break;
			}
			case 'p':
			{
				port = atoi(optarg);
				modem_log("Port base selected is %d", port);
			}
			default:break;
		}  
	}  
	
	if(device == NULL)
	{
		device = DEFAULT_DEVICE;
	}
	signal(SIGINT, exit_handler);
	if(modem_setup(&settings, device, port) == 0)
	{
		
		pthread_create(&settings.sock_thread, NULL, modem_udp_server_task, (void*) &settings);
		modem_tty_server_task((void*)&settings);

		close(settings.tty_fd);
		close(settings.sock_fd);

	}
	
	return 0;
}

void *modem_tty_server_task(void *arg)
{
	config_settings_t *cfg = (config_settings_t*) arg;
	char buffer[80];
	cfg->run = 1;
	while(cfg->run)
	{
		int count = read(cfg->tty_fd, buffer, sizeof(buffer));
		buffer[count] = '\0';
		if(count)
		{
			sendto(cfg->sock_fd, (const char *)buffer, count, MSG_CONFIRM, (const struct sockaddr *) &cfg->client, cfg->clilen); 
			modem_log("UDP TX: '%s'", buffer);		
		}
	}
	write(cfg->tty_fd, "hello", 5);
	
	return NULL;
}

void *modem_udp_server_task(void *arg)
{
	config_settings_t *cfg = (config_settings_t*) arg;
	char buffer[80];
	cfg->run = 1;
	while(cfg->run)
	{
		int count = recvfrom(cfg->sock_fd, (char *)buffer, sizeof(buffer), MSG_WAITALL, ( struct sockaddr *) &cfg->client, &cfg->clilen); 
		buffer[count] = '\0';
		modem_log("UDP RX: '%s'", buffer);
		write(cfg->tty_fd, buffer, count);
	}

	return NULL;
}

int modem_setup(config_settings_t *cfg, char *device, int port)
{
	
	struct termios tty;
	struct sockaddr_in servaddr;
	
	/* setup the TTY */

	cfg->tty_fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
	if(cfg->tty_fd < 0)
	{
		modem_log("Failed to open device '%s': %s", device, strerror(errno));
		return -1;
	}
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (cfg->tty_fd, &tty) != 0)
    {
            modem_log("error %d from tcgetattr", errno);
            return -1;
    }

    cfsetospeed (&tty, B115200);
    cfsetispeed (&tty, B115200);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;         
    tty.c_lflag = 0;                                
    tty.c_oflag = 0;              
    tty.c_cc[VMIN]  = 0;          
    tty.c_cc[VTIME] = 5;          

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    tty.c_cflag |= (CLOCAL | CREAD);

    tty.c_cflag &= ~(PARENB | PARODD);      
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr (cfg->tty_fd, TCSANOW, &tty) != 0)
    {
            modem_log("error %d from tcsetattr", errno);
            return -1;
    }

	
	/* setup the UDP socket */
	
	memset(&servaddr, 0, sizeof(servaddr)); 

	servaddr.sin_family    = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(port); 

	cfg->sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(cfg->sock_fd < 0)
	{
		modem_log("failed to open socket (%d)", cfg->sock_fd);
		return -1;
	}

	if ( bind(cfg->sock_fd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ) 
	{
		modem_log("failed to bind socket (%d)", cfg->sock_fd);
		return -1;
		
	}


	return 0;

}


void exit_handler(int sig)
{
	settings.run = 0;
}



void modem_log(char *fmt, ...)
{
	va_list lst;
	char modem_log_buffer[80];
	va_start(lst, fmt);
		
	vsnprintf(modem_log_buffer, sizeof(modem_log_buffer), fmt, lst);
	
	va_end(lst);
	/* TODO: mutex */
	printf("%s\r\n", modem_log_buffer);

}
