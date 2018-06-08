/*
Author: Allen Olasunkanmi
Assignment #: 5
client.c
Sources: Dr. Hyatts notes in class and Unix programming Text book
Note: This runs in a LinuX Environment.

In order to cause packets to be missing this will be done deliberately.
That is, a number of randomly selected packets will be deliberately ommited 
and the server will be setup to determine this occurence and send a 
request to the client for retransmission.
This will be the initial approach. A second approach would be to cause an
overflow at the server's buffer. This is not for cetain yet.
*/
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#define MAXLINE 1024

main(int argc, char *argv[]){
  int n, i, clientSocket, bytes, serverlen;
  char recvline[MAXLINE+1], sendline[MAXLINE];  
  struct sockaddr_in servAddr;
  //socklen_t serverlen;
  
  if(argc != 3){
    printf("usage: <client name> <Port Number> <message>\n");
    exit(1);
  }  
  printf("Enter 'quit' to exit.\n");
  // Set up socket for UDP packets
  if ( (clientSocket = socket(AF_INET, SOCK_DGRAM, 0) ) <0 ){
    printf("Socket error.\n");
    exit(1);
  }  
  // Set up socket to for connecting to server
  bzero(&servAddr, sizeof(servAddr.sin_addr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_port   = htons(atoi(argv[1]));
  
  if(inet_pton (AF_INET, argv[1], &servAddr.sin_addr ) < 0){
	  printf("Connection Error @ port #: %s. \n", argv[1]);
    exit(1);
  }
  strcpy(sendline, argv[2]);
  
  //printf("clientSocket = %d \n", clientSocket);
  // send & recieve UPD datagrams to server
  serverlen = sizeof(servAddr);
 bytes = sendto(clientSocket, sendline, strlen(sendline),0,(struct sockaddr*)&servAddr, serverlen);
  ///*{
  if (bytes < 0){
	  printf("Bad send.\n");
	  exit(-1);
    }
  //}*/
  serverlen = sizeof(servAddr);
  if ( (bytes = recvfrom( clientSocket, recvline,MAXLINE,0,  
    (struct sockaddr*)&servAddr, &serverlen  )) < 0 )
  {
	  printf("Bad receive.\n");
	  exit(-1);
  }
  printf("read %d bytes \n",bytes);
  printf("read %s \n",recvline);
  
  close(clientSocket);
  return(0);
}