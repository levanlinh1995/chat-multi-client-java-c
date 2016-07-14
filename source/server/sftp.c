#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <semaphore.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

#include "chatroom_utils.h"
#include "log.h"

#define MAX_CLIENTS 100

void initialize_server(connection_info *server_info, int port);
int  Accept_Server_connection(connection_info *server_info, connection_info clients[]);
void* handle_client_message(void* sock);
void connect_to_server(connection_info *connection, char *address, char *port);
void send_port_server(connection_info *connection,char* port);

// khai bao bien toan cuc
user userList[MAX_CLIENTS];
connection_info clients[MAX_CLIENTS];
char *buffer;

sem_t sem;

int main(int argc, char *argv[])
{
  puts("Starting server.");
  int sock_client;
  int rc;
  buffer = (char*) malloc(sizeof(char) * 500000);
  connection_info server_info;
  connection_info connection;	
  
  int i;
  for(i = 0; i < MAX_CLIENTS; i++)
  {
    clients[i].socket = 0;
  }

  for(i = 0; i < MAX_CLIENTS; i++)
  {
  		bzero(userList[i].username, sizeof(userList[i].username));
  		bzero(userList[i].password, sizeof(userList[i].password));
  		bzero(userList[i].fullname, sizeof(userList[i].fullname));
  }

  if (argc != 2)
  {
      fprintf(stderr, "Usage: %s <port>\n", argv[0]);
      exit(1);
  }

  //khoi tao sem
  if(sem_init(&sem,0,0)==-1)
        perror("Semaphore initialize failed");

    // khoi tao server, lang nghe ket noi client
  initialize_server(&server_info, atoi(argv[1]));

  pthread_t tid;
  connect_to_server(&connection, "127.0.0.1", "9999");
  send_port_server(&connection, argv[1]);
	
  while(true)
  {
      // chap nhan ket noi tu client
      sock_client = Accept_Server_connection(&server_info, clients);
      printf("\n----- Client connected!------\n");

      rc = pthread_create(&tid, NULL, (void*)handle_client_message, (void*)&sock_client);
      if (rc) 
      {              
          perror("ERROR; return code from pthread_create()");
          exit(1);
      }
  }

  sem_destroy(&sem);
  pthread_exit(NULL);
  return 0;
}

void initialize_server(connection_info *server_info, int port)
{
    if((server_info->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Failed to create socket");
        exit(1);
    }

    server_info->address.sin_family = AF_INET;
    server_info->address.sin_addr.s_addr = INADDR_ANY;
    server_info->address.sin_port = htons(port);


    if(bind(server_info->socket, (struct sockaddr *)&server_info->address, sizeof(server_info->address)) < 0)
    {
        perror("Binding failed");
        exit(1);
    }

    const int optVal = 1;
    const socklen_t optLen = sizeof(optVal);
    if(setsockopt(server_info->socket, SOL_SOCKET, SO_REUSEADDR, (void*) &optVal, optLen) < 0)
    {
        perror("Set socket option failed");
        exit(1);
    }


    if(listen(server_info->socket, 3) < 0) 
    {
        perror("Listen failed");
        exit(1);
    }

    //Accept and incoming connection
    printf("Waiting for incoming connections...\n");
}

int  Accept_Server_connection(connection_info *server_info, connection_info clients[])
{
  int new_socket;
  int address_len;
  new_socket = accept(server_info->socket, (struct sockaddr*)&server_info->address, (socklen_t*)&address_len);

  if (new_socket < 0)
  {
    perror("Accept Failed");
    exit(1);
  }

  int i;
  for(i = 0; i < MAX_CLIENTS; i++)
  {
      if(clients[i].socket == 0) 
      {
        clients[i].socket = new_socket;
        break;

      } else if (i == MAX_CLIENTS -1) // if we can accept no more clients
      {
        //send_too_full_message(new_socket);
      }
  }
  return new_socket;
}

void* handle_client_message(void* sock)
{
	int socket = *(int*)sock;
	int socketRecv;
	int read_size;
	char msg[5000];

  	bzero((char*)msg,sizeof(msg));

	
	while((read_size = recv(socket, msg, sizeof(msg), 0)) > 0)
	{
    	printf("msg to clients: %s\n", msg);

		char *type = NULL;
		type = strtok(msg,"/");
		if (strcmp(type, "SEND") == 0 )
	    {
	    	type = strtok(NULL,"/");
	    	strcpy(buffer,type);
	      	sem_post(&sem);
	    }

	    if (strcmp(type, "RECV") == 0 )
	    {
	    	socketRecv = socket;
	      	sem_wait(&sem);
	      	send(socketRecv,buffer,strlen(buffer),0);
	    }
  	}
    if(read_size == 0)
    {
    	puts("Client disconnected");
    	fflush(stdout);
    }
    else if(read_size == -1)
    {
    	int i = 0;
    	for( i = 0; i < MAX_CLIENTS; i++)
    	{
    		if(clients[i].socket == socket)
    		{
    			clients[i].socket = 0;
    			break;
    		}

    	}
    	perror("Client disconnected");
    }

    
    pthread_exit(NULL);  
}
void connect_to_server(connection_info *connection, char *address, char *port)
{
      //Create socket
      if ((connection->socket = socket(AF_INET, SOCK_STREAM , IPPROTO_TCP)) < 0)
      {
          perror("Could not create socket");
      }

      connection->address.sin_addr.s_addr = inet_addr(address);
      connection->address.sin_family = AF_INET;
      connection->address.sin_port = htons(atoi(port));

      //Connect to remote server
      if (connect(connection->socket, (struct sockaddr *)&connection->address , sizeof(connection->address)) < 0)
      {
          perror("Connect failed.");
          exit(1);
      }
}

void send_port_server(connection_info *connection,char* port)
{
    char msg[3000];
    strcat(msg, "PORT/");
    strcat(msg, port);
    strcat(msg, "/");

    if(send(connection->socket, msg, strlen(msg), 0) < 0)
    {
        perror("Send failed");
        exit(1);
    }
    printf("Sftp day ne: %s\n",msg);
}