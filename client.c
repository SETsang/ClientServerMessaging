/*****************************
 * UDP, Client
 * Spencer Tsang 
 * 25 March 2023
 *****************************/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include "tfv2.h"
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

int main (int argc, char *argv[])
{
	// Variables such as port number, number of bytes, and socket number
	int sock, portNum, nBytes;
	char buffer[10];
	struct sockaddr_in serverAddr;
	socklen_t addr_size;
	int state = 0;
	int i;

	// Time variables
	int timeCounter = 0;
	int ACKcounter = 0;
	int counter = 0;
	size_t read_bytes;

	// Timer variables
	srand(time(NULL));

	struct timeval tv;
	int rv;
	fd_set readfds;
	fcntl(sock, F_SETFL, O_NONBLOCK);

	if (argc != 5)
	{
		printf ("Usage: %s <port> <host> <source file> <destination file>\n", argv[0]);
		return 1;
	}

	// Configure address
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons (atoi (argv[1]));
	inet_pton (AF_INET, argv[2], &serverAddr.sin_addr.s_addr);
	memset (serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));  
	addr_size = sizeof serverAddr;

	// Create UDP socket
    	if ((sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0) // Socket errors
	{
		printf ("socket error\n");
		return 1;
	}

	// init file packet
	PACKET filePack;
	PACKET ack;
	strncpy(filePack.data, argv[4], SIZE);
	filePack.header.seq_ack = 0;
	filePack.header.length = sizeof(filePack.data);
	filePack.header.checksum = 0;
	filePack.header.checksum = calculateChecksum(&filePack);

	// Send filePacket
	while(state == 0){
		sendto(sock, &filePack, sizeof(filePack), 0, (struct sockaddr *)&serverAddr, addr_size);
		printf("Sent file: %s\n", filePack.data);

		// Received file ACK
		size_t read_bytes = recvfrom(sock, &ack, sizeof(ack), 0, NULL, NULL);
		
		if(ack.header.seq_ack == 0){
			state = 1;
			break;
		}
		counter++;
		if(counter == 3){
			printf("Three failures\n");
			return 1;
		}	
		printf("*Received NAK...Resending*\n");
	}

	// Open File
	FILE *fp = fopen(argv[3], "rb");
	if(fp == NULL)
		printf("File cannot be opened");
	int flag = 0;	
	// Send over data from file
	while((read_bytes = fread(buffer, 1, 10, fp)) > 0){
			
		while(1){	
			if(ACKcounter == 3){
				fclose(fp);
				close(sock);
				break;
			}	
			
			// init packet
			PACKET pack;
			strncpy(pack.data, buffer, read_bytes);
			pack.header.length = read_bytes;
			pack.header.seq_ack = state;
			
			// Randomizer to simulate packet loss
			int random = rand() % 10;
			if(random == 1){
				pack.header.checksum = 0;
			}else{
				pack.header.checksum = calculateChecksum(&pack);	
			}
		

			// Send file data to server
			sendto (sock, &pack, sizeof(pack), 0, (struct sockaddr *)&serverAddr, addr_size);
			printf("Sending data: %s\n", pack.data);
			

			// Timer setup
			FD_ZERO(&readfds);
			FD_SET(sock, &readfds);
			tv.tv_sec = 2;
			tv.tv_usec = 0;
			rv = select(sock+1,&readfds,NULL,NULL,&tv);

			// Receive file
			PACKET ack;
			
			if(rv == 0){
				printf("Timed out... resending packet\n");
				timeCounter++;
				if(timeCounter == 3){
					fclose(fp);
					close(sock);
					break;
				}
				continue;
			}

			else if(rv == 1){
				recvfrom(sock, &ack, sizeof(ack), 0, NULL, NULL);
				timeCounter = 0;
			}

			if(ack.header.seq_ack == state){
				if(state == 1){
					state = 0;
				}else{
				   	state = 1;
				}
				ACKcounter = 0;
				break;
			}
			printf("Received NAK...Resending...\n");
			ACKcounter++;
		}
		memset(buffer, '\0', sizeof(buffer));
	}	
	
	PACKET dc; 
	dc.header.length = 0;
	memset(dc.data, '\0', dc.header.length);
	for(i = 0; i < 3; ++i){
		sendto (sock, &dc, sizeof(dc), 0, (struct sockaddr *)&serverAddr, addr_size);
	}

	fclose(fp);
	close(sock);

	return 0;
}
