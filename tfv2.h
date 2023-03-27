/*****************************
 * tfv2.h
 * Spencer Tsang 
 * 25 March 2023
 *****************************/
 
#include <stdio.h>
#include <fcntl.h> 
#include <unistd.h> 

#define SIZE 256

typedef struct{
	int seq_ack; // 1 or 0
	int length; // How many bytes of data you have
	int checksum; // Checksum calculated by XORing bytes in packet
} HEADER;

typedef struct{
	HEADER header;
	char data[SIZE];
} PACKET;

int calculateChecksum(PACKET *pkt){
	char checkBuff = 0;
	char *p = (char*)pkt;
	int i;
	for(i=0;i<sizeof(p);++i){
		checkBuff ^= *p++;
	}
	return (int)checkBuff;
}
