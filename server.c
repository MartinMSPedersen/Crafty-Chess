/*
Author: Allen Olasunkanmi
Assignment #: 5
Server.c
Note: This runs in a Linux Environment.
This is a UDP server.
It will look out for missed UPD packets and send a resend
request to the client if it finds a missing packet.

*/
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#define MAXLINE 1024
#define MAXSOCKETS 5

main(int argc, char *argv[]){
  if(argc != 2){
    printf("usage: <Server name> <Port Number>\n");
    exit(1);
  }  
  int i, x, serverSocket, socketToClient, fromlen, msglength;
  struct sockaddr_in servAddr, clientAddr;
  char buff[MAXLINE];

  // Prepare to accept UPD packets
  if ((serverSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ){
	  printf("Server cannot create a passive connection.\n");
  }
  bzero(&servAddr, sizeof(servAddr) );
  servAddr.sin_family = AF_INET;  
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(atoi(argv[1])); 
 
  if ( bind (serverSocket, (struct sockaddr*) &servAddr, sizeof(servAddr) ) < 0 ){
    printf("Unable to bind socket.\n");	  
	  exit(1);
  }
 
  for ( ; ; )
  {
    printf("SERVER: Accepting UDP packets.\n");	
    fromlen = sizeof(clientAddr);// Reset buffer      
		// Read packets sent from the client; exit if the right condition is met
		if ( (msglength = recvfrom(serverSocket, buff,MAXLINE, 0,
      (struct sockaddr*) &clientAddr, &fromlen)) < 0 )
		//||(strstr(buff,"quit")) )
		{
			printf("Closing connection. \n");
			close(serverSocket);
			serverSocket = 0;
			exit(0);
		}
		buff[msglength]=0; // Null terminate.
		printf("SERVER: read %d bytes clientAddr ***  ",msglength );
		printf("SERVER: message = %s \n",buff);	

    // Send packet to the client
		if ( (msglength = sendto(serverSocket, buff, msglength/*(strlen(buff)+1)*/ ,0, 
      (struct sockaddr *)&clientAddr, sizeof(clientAddr)))< 0)
		{
			printf("SERVER: Bad send \n ");
			exit(1);
		}    
  }
  // close all open sockets
  close(socketToClient);
  return (0);
}