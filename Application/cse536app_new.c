#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>

#define SERVER_PORT 23456 
#define MAX_PENDING 5
#define MAX_LINE 256


int sendMsg(char *data)
{
   struct sockaddr_in client, server;
   struct hostent *hp;
   char buf[MAX_LINE];
   int len, ret, n;
   int s, new_s;
   memcpy(buf,data,256);
   bzero((char *)&server, sizeof(server));
   server.sin_family = AF_INET;
   server.sin_addr.s_addr = INADDR_ANY;
   server.sin_port = htons(0);

   s = socket(AF_INET, SOCK_DGRAM, 0);
   if (s < 0)
   {
		perror("simplex-talk: UDP_socket error");
		exit(1);
   }

   if ((bind(s, (struct sockaddr *)&server, sizeof(server))) < 0)
   {
		perror("simplex-talk: UDP_bind error");
		exit(1);
   }

   hp = gethostbyname( "192.168.0.37" );
   if( !hp )
   {
      	fprintf(stderr, "Unknown host %s\n", "localhost");
      	exit(1);
   }

   bzero( (char *)&server, sizeof(server));
   server.sin_family = AF_INET;
   bcopy( hp->h_addr, (char *)&server.sin_addr, hp->h_length );
   server.sin_port = htons(SERVER_PORT); 

  
		buf[MAX_LINE-1] = '\0';
	      len = strlen(buf) + 1;
		n = strlen(buf);
		//printf("inside Hell");
	      ret = sendto(s, buf, n, 0,(struct sockaddr *)&server, sizeof(server));
		if( ret != n)
		{
			fprintf( stderr, "Datagram Send error %d\n", ret );
			exit(1);
		}
   
   return 0;
}

int main()
{
	FILE *fd = NULL;
	//char buffer[128]="192.168.32.134	Hello World\n";
	char ipaddress[20];
	char data[236];
	int exit = 0;
	size_t count;
	fd = fopen("/dev/cse5361", "rb+");
	setbuf(fd,NULL);
	//fwrite(buffer, 1, strlen(buffer), fd);
	while(exit!=1){
		int choice;int ret;
		char message[256];
		char log[256];
		printf("Please select any of the below options: \n 1. Send Message \n 2. Recieve Messages \n 3. exit\n");
		scanf("%d",&choice);
		switch(choice)
		{
			case 1: 
			printf("\nEnter ip address:");
			scanf("%s",ipaddress);
			printf("\n Enter Message:");
			scanf("%s",data);
			sprintf(log,"SatyaSwaroopB: Sending to %s \n Message is %s \n", ipaddress,data);
			sendMsg(log);
			strcpy(message,ipaddress);
			strcat(message,"\t");
			strcat(message,data);
			
			ret=fwrite(message, 1, 256, fd);
			printf("Retrun: %d\n",ret);
			
			choice = 4;	
			break;
			case 2:
			count = fread(message, 1, 256, fd);
			sprintf(log,"Recieved: %s\n", message);
			sendMsg(log);
			//sendMsg(message);
			printf("bytes read: %s\n", message);
			break;
			case 3:
			printf("Bye\n");
			exit=1;
			break;
			default:
			continue;
		}

	}	
	fclose(fd);
}
