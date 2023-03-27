/*****************************
 * UDP, Server
 * Spencer Tsang 
 * 25 March 2023
 *****************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include "tfv2.h"
#include <time.h>

int main (int argc, char *argv[])
{
	// Variables such as port number, number of bytes, and socket number
	int sock, nBytes;
	char buffer[10];
	struct sockaddr_in serverAddr, clientAddr;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size, client_addr_size;
	int i;
	int counter = 0;
	PACKET pack, filePack;
	srand(time(0));

	if (argc != 2)
    	{
        	printf ("Usage: %s <port>\n", argv[0]);
        	return 1;
    	}

	// init
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons ((short)atoi (argv[1]));
	serverAddr.sin_addr.s_addr = htonl (INADDR_ANY);
	memset ((char *)serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));  
	addr_size = sizeof (serverStorage);

	// Create a socket
	if ((sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf ("socket error\n");
		return 1;
	}

	// Bind
	if (bind (sock, (struct sockaddr *)&serverAddr, sizeof (serverAddr)) != 0)
	{
		printf ("bind error\n");
		return 1;
	}
	
	int fileOpened = 0;	
	
	// File implementation
	while(1){
		recvfrom(sock, &filePack, sizeof(filePack), 0, (struct sockaddr *)&serverStorage, &addr_size);
		
		// Receive file packet
		printf("Received File: %s\n", filePack.data);

		// Calculate checksums
		int received_checksum = filePack.header.checksum;
		filePack.header.checksum = 0;
		int calculated_checksum = calculateChecksum(&filePack);

		// Compare checksums, send ack if same
		if(received_checksum == calculated_checksum){	
			
			//init ack
			PACKET ack;
			ack.header.length = 0;
			ack.header.seq_ack = filePack.header.seq_ack;
			sendto (sock, &ack, sizeof(ack), 0, (struct sockaddr *)&serverStorage, addr_size);
			fileOpened = 1;
			printf("Sending ACK...\n");
		}else{
				
			// nak
			PACKET nak;
			
			// Simulates packet loss
			if(nak.header.seq_ack == 0)
				nak.header.seq_ack = 1;
			else
				nak.header.seq_ack = 0;	
				
			nak.header.length = 0;
			sendto (sock, &nak, sizeof(nak), 0, (struct sockaddr *)&serverStorage, addr_size);
			printf("*File NAK*\n");
		}
		if(fileOpened == 1)
			break;
	}

	FILE *fp = fopen(filePack.data, "wb");	

	while(1){
		// Receive file packets
		recvfrom(sock, &pack, sizeof(pack), 0, (struct sockaddr *)&serverStorage, &addr_size);

		if(pack.header.length == 0){
			fclose(fp);
			close(sock);
			return 1;
		}

		// Calculate checksums
		int received_checksum = pack.header.checksum;
		int calculated_checksum = calculateChecksum(&pack);
		int random = rand() % 7;

		// Random packet loss for testing
		if(random == 1)
			continue;
		
		// Compare checksums, send ack if same
		if(received_checksum == calculated_checksum){
			
			// Write data
			fwrite(pack.data, sizeof(char), pack.header.length, fp);
	
			// ack
			PACKET ack;
			ack.header.length = 0;
			ack.header.seq_ack = pack.header.seq_ack;
			sendto (sock, &ack, sizeof(ack), 0, (struct sockaddr *)&serverStorage, addr_size);
			counter = 0;
			printf("Sending ACK...\n");
		
		}else{
			
			// init nak
			PACKET nak;
			random = rand() % 7;

			// Simulates the wrong sequence change
			if(random == 1){
				continue;
			}

			if(pack.header.seq_ack == 0){
				nak.header.seq_ack = 1;
			}

			else{
				nak.header.seq_ack = 0;	
			}
			
			counter++;
		
			if(counter == 3){
				fclose(fp);
				close(sock);
				return 1;
			}
		
			sendto (sock, &nak, sizeof(nak), 0, (struct sockaddr *)&serverStorage, addr_size);
			printf("*Sent NAK*\n");
		}
	}
	return 0;
}
