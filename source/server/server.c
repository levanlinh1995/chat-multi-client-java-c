/*
*
* Chatroom - a simple linux commandline client/server C program for group chat.
* Author: Andrew Herriot
* License: Public Domain
*
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>

#include <pthread.h>

#include "chatroom_utils.h"
#include "log.h"

#define MAX_CLIENTS 100


void showMenu();
void initialize_server(connection_info *server_info, int port);
void login(int socket,char *username, char * password, char* ip, char* port);
void Register(pid_t   childpid,int socket,char * fullname,char * username, char * password);
int  Accept_new_connection(connection_info *server_info, connection_info clients[]);
void* handle_client_message(void* sock);


// khai bao bien toan cuc
user* userList;
connection_info* clients;
char portServerT[10];
int socketofServerT;
sem_t sem;

void reverse(char s[])
 {
     int i, j;
     char c;
 
     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
 }

 void itoa(int n, char s[])
 {
     int i, sign;
 
     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
 }

int main(int argc, char *argv[])
{
    /*Shared memory */
	int segment_id1;
	int segment_id2;
    

    /* Allocate a shared memory segment. */
    
	if ((segment_id1 = shmget(IPC_PRIVATE, MAX_CLIENTS * sizeof(user), SHM_R | SHM_W )) == -1) {
		perror("shmget");
		logline(LOG_ERROR, "can not call shmget");
		exit(1);
	}
	if ((segment_id2 = shmget(IPC_PRIVATE, MAX_CLIENTS * sizeof(connection_info), SHM_R | SHM_W)) == -1) {
		perror("shmget");
		logline(LOG_ERROR, "can not call shmget");
		exit(1);
	}

     /* Attach the shared memory segment. */
    
	userList =  shmat (segment_id1, NULL, 0);
	clients =  shmat (segment_id2, NULL, 0);    
    
	// Khoi tao semaphore
	if(sem_init(&sem,0,0)==-1)
	{
        perror("Semaphore initialize failed");
        logline(LOG_ERROR, "can not semaphore init");
	}
	

	int sock_client;
	int rc;


	puts("Starting server.");
	logline(LOG_INFO, "Starting server.");
	connection_info server_info;



	int i;
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		clients[i].socket = 0;
		bzero(clients[i].username,sizeof(clients[i].username));
		bzero(clients[i].ip,sizeof(clients[i].ip));
		bzero(clients[i].port,sizeof(clients[i].port));


	}
	for( i = 0; i< MAX_CLIENTS; i++)
	{
		bzero(userList[i].username,sizeof(userList[i].username));
		bzero(userList[i].password,sizeof(userList[i].password));
		bzero(userList[i].fullname,sizeof(userList[i].fullname));

	}


	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <port>\n", argv[0]);
		logline(LOG_INFO, "Usage: %s <port>", argv[0]);
		exit(1);
	}

  // khoi tao server, lang nghe ket noi client
	initialize_server(&server_info, atoi(argv[1]));
	
	pthread_t tid;
	
	for(;;)
	{
        // chap nhan ket noi tu client
		sock_client = Accept_new_connection(&server_info, clients);
		logline(LOG_INFO, "Client connect success.");
		printf("\n----- Client connected! -----\n");
		rc = pthread_create(&tid, NULL, (void*)handle_client_message, (void*)&sock_client);
		if (rc) 
		{              
			perror("ERROR: return code from pthread_create()");
			logline(LOG_ERROR,"ERROR: return code from pthread_create()" );
			exit(1);
		}
	}

	////////////////
	shmdt(userList);
    shmdt(clients);
    sem_destroy(&sem);
	pthread_exit(NULL);

	return 0;
}

void initialize_server(connection_info *server_info, int port)
{
	if((server_info->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Failed to create socket");
		logline(LOG_ERROR, " Failed to create socket");
		exit(1);
	}

	server_info->address.sin_family = AF_INET;
	server_info->address.sin_addr.s_addr = INADDR_ANY;
	server_info->address.sin_port = htons(port);

	if(bind(server_info->socket, (struct sockaddr *)&server_info->address, sizeof(server_info->address)) < 0)
	{
		perror("Binding failed");
		logline(LOG_ERROR, " Blinding failed");
		exit(1);
	}

	const int optVal = 1;
	const socklen_t optLen = sizeof(optVal);
	if(setsockopt(server_info->socket, SOL_SOCKET, SO_REUSEADDR, (void*) &optVal, optLen) < 0)
	{
		perror("Set socket option failed");
		logline(LOG_ERROR, "Set socket option failed");
		exit(1);
	}


	if(listen(server_info->socket, 100) < 0) {
		perror("Listen failed");
		logline(LOG_ERROR, "Listen failed");
		exit(1);
	}

  //Accept and incoming connection
	printf("Waiting for incoming connections...\n");
}

void send_public_message(int socket, char* ContentMsg)
{
	int i;
	char  msg[3000];
	bzero((char*)msg,sizeof(msg));
	strcat(msg, "PUBLIC_MESSAGE/");
	strcat(msg, ContentMsg);
	strcat(msg, "/");
    // ten nguoi gui
	for( i = 1; i < MAX_CLIENTS ; i++)
	{
		if(clients[i].socket == socket)
		{
			strcat(msg, clients[i].username);
			strcat(msg, "/");
			break;
		}
	}

	strcat(msg, "\n");
	for(i = 1; i < MAX_CLIENTS; i++)
	{
		if(clients[i].socket != 0 )
		{
			if(send(clients[i].socket, msg, strlen(msg), 0) < 0)
			{
				perror("Send failed(Public)");
				logline(LOG_ERROR, "Send failed");
				exit(1);
			}

		}
	}
}

void send_port_client(int socket, char* receiver, char* portServerT)
{
	int i;
	int sock = 0;
	char msg[3000], msg1[3000];
	 // chuyen socketS sang chuoi (socketbuf)

	bzero((char*)msg,sizeof(msg));
	bzero((char*)msg1,sizeof(msg1));
	//bzero((char*)msg2,sizeof(msg2));
	strcat(msg, "PORT/RECV/");
	strcat(msg, portServerT);
	strcat(msg, "/");

	strcat(msg1, "PORT/SEND/");
	strcat(msg1, portServerT);
	strcat(msg1, "/");

	
	//Tim nguoi nhan
	for(i = 1; i < MAX_CLIENTS; i++)
	{
		if(strcmp(clients[i].username, receiver)==0 )
		{
			sock = clients[i].socket;
			break;
		}
	};
	strcat(msg, "\n");
	strcat(msg1, "\n");

	printf("Send_msgport_client: %s ", msg);
	printf("Send_msg1port_client: %s ", msg1);

	//Socket nguoi nhan
	if(send(sock, msg, strlen(msg), 0) < 0)
	{
		perror("Send failed");
		logline(LOG_ERROR, "send failed");
		exit(1);
	}
	//Socket, socket cua nugoi gui -> Show ngay bang cua minh
	if(send(socket, msg1, strlen(msg1), 0) < 0)
	{
		perror("Send failed");
		exit(1);
	}

}
void send_setup_client(int socket, char* receiver, char* filename)
{
	int i;
	int sock = 0;
	char msg[3000];
	bzero((char*)msg,sizeof(msg));
	strcat(msg , "SETUPSENDFILE/");
	strcat(msg, filename);
	strcat(msg,"/");


	//Tim nguoi gui
	for(i = 1; i < MAX_CLIENTS; i++)
	{
		if(clients[i].socket == socket)
		{
			strcat(msg, clients[i].username);
			strcat(msg,"/");
			break;
		}
	}

	//Tim nguoi nhan
	for(i = 1; i < MAX_CLIENTS; i++)
	{
		if(strcmp(clients[i].username, receiver)==0 )
		{
			sock = clients[i].socket;
			break;
		}
	}
	strcat(msg, "\n");
	printf("send_Setup_client: %s",msg);
	//msg: FILE/Nguoi gui(clients[i].username)/
	//Sock, socket cua nguoi nhan
	if(send(sock, msg, strlen(msg), 0) < 0)
	{
		perror("Send failed(setup1)");
		logline(LOG_ERROR, "Send failed");
		exit(1);
	}
}
void send_private_message(int socket, char* ContentMsg, char* receiver)
{
	int i;
	int sock = 0 ;
	char  msg[3000];
	bzero((char*)msg,sizeof(msg));
	strcat(msg, "PRIVATE_MESSAGE/");
	strcat(msg, ContentMsg);
	strcat(msg, "/");

	for(i = 1; i < MAX_CLIENTS ; i++)
	{
		if(clients[i].socket == socket)
		{
			strcat(msg, clients[i].username);
			strcat(msg, "/");
			break;
		}
	}


	for(i = 1; i < MAX_CLIENTS; i++)
	{
		if(strcmp(clients[i].username, receiver)==0 )
		{
			sock = clients[i].socket;
			break;
		}
	}
	


	strcat(msg, "\n");
	if(send(sock, msg, strlen(msg), 0) < 0)
	{
		perror("Send failed(Private1)");
		logline(LOG_ERROR,"Send failed");
		exit(1);
	}

	if(send(socket, msg, strlen(msg), 0) < 0)
	{
		perror("Send failed(Private2)");
		logline(LOG_ERROR,"Send failed");
		exit(1);
	}
}

void send_user_online_list(int socket) 
{
	char  list[5000];
	bzero((char*)list,sizeof(list));
	strcat(list, "ONLINE_LIST/");

	int i;
	for(i = 1; i < MAX_CLIENTS; i++)
	{
		if(clients[i].socket != 0)
		{
			if( strlen(clients[i].username) != 0)
			{
				strcat(list, clients[i].username);
				strcat(list, "/");
			}
		}
	}
	strcat(list,"\n" );

	if(send(socket, list, strlen(list), 0) < 0)
	{
		perror("Send failed(Online)");
		logline(LOG_ERROR,"Send failed");
		exit(1);
	}
}
void send_user_disconnect(int socket)
{
	char  list[5000];
	bzero((char*)list,sizeof(list));
	strcat(list, "DISCONNECT/");

	int i;
	for(i = 1; i < MAX_CLIENTS; i++)
	{
		if(clients[i].socket == socket)
		{
			strcat(list, clients[i].username);
			strcat(list, "/");
			break;
		}
	}
	strcat(list,"\n" );

	// gui user disconnect toi client
	for(i = 1; i < MAX_CLIENTS ; i++)
	{
		if(clients[i].socket != 0 && clients[i].socket != socket)
		{
			if( send(clients[i].socket, list, strlen(list), 0) < 0)
			{
				perror("Send failed(Disconect)");
				logline(LOG_ERROR,"Send failed");
				exit(1);
			}
		}
	}	
}


void logout(int socket)
{
	int i;
	for(i = 1; i < MAX_CLIENTS; i++)
	{
		if(clients[i].socket == socket)
		{
			clients[i].socket = 0;
			bzero(clients[i].username, sizeof(clients[i].username));
		}
	}

	char msg[] = "LOGOUT_SUCCESS/\n";
    if(send(socket, msg, sizeof(msg), 0) < 0)
    {
        perror("Send failed(logout)");
        logline(LOG_ERROR,"Send failed");
        exit(1);
    }

    return;
}
void login(int socket,char *username, char * password, char * ip, char* port)
{
    // kiem tra xem user va pass co dung khong
	int i;

	for( i = 1; i < MAX_CLIENTS; i++)
	{
		if(strlen(userList[i].username) != 0)
		{

          if( (strcmp(userList[i].username,username)==0) && (strcmp(userList[i].password,password)==0) ) // khop
          {
            // them vao phien lam viec
          	clients[i].socket = socket;
          	strcpy(clients[i].username, username);
          	strcpy(clients[i].ip, ip);
          	strcpy(clients[i].port, port);


            // show danh sach online
          	showMenu();
          	logline(LOG_INFO,"Client %s login success.",username);

            // gui thong bao thanh cong cho client

          	char msg[] = "LOGIN_SUCCESS/\n";
          	if(send(socket, msg, sizeof(msg), 0) < 0)
          	{
          		perror("Send failed(loginS)");
          		logline(LOG_ERROR,"Send failed");
          		exit(1);
          	}

          	return;

          }
      }
  }
        // gui thong bao khong thanh cong
  logline(LOG_ERROR,"Client %s login unsuccess.",username);
  char msg[] = "LOGIN_FAIL/\n";
  if(send(socket, msg, strlen(msg), 0) < 0)
  {
  	perror("Send failed(loginF)");
  	logline(LOG_ERROR,"Send failed");
  	exit(1);
  }

}
void Register( pid_t   childpid, int socket,char * fullname,char * username, char * password)
{
	int i;

	for(i = 1; i < MAX_CLIENTS; i++)
	{
        if(strlen(userList[i].username) == 0) // rong
        {
              // dang ki, them vao database
        	
        	strcpy(userList[i].username,username);
        	strcpy(userList[i].password,password);
        	strcpy(userList[i].fullname,fullname);
        	clients[i].socket = 0;
            
            logline(LOG_INFO,"Client %s Register success.",username);
            
        	int n;
        	char msg[] = "REGISTER_SUCCESS/\n";
        	n = write(socket,msg,strlen(msg));
        	if (n < 0)
        	{
        		perror("ERROR Send to socket");
        		logline(LOG_ERROR,"Send failed");
        		exit(1);
        	}
        	break;
        }
        else
        {
        	if( strcmp(userList[i].username, username) == 0)
        	{
                 // gui thong bao fail
        		logline(LOG_ERROR,"Client %s Register unsuccess.",username);
        		int n;
        		char msg[] = "REGISTER_FAIL/\n";
        		n = write(socket,msg,strlen(msg));
        		if (n < 0)
        		{
        			perror("ERROR Send to socket");
        			logline(LOG_ERROR,"Send failed");
        			exit(1);
        		}
        		break;
        	}
        }
    }
    
    return;
}
void changeAcc(int socket,char * CurrentUsername,char * CurrentPass, char * newFullname, char * newUsername, char * newPassword)
{

    // kiem tra xem user va pass co dung khong
	int i;

	for( i = 1; i < MAX_CLIENTS; i++)
	{
		if(strlen(userList[i].username) != 0)
		{

          if( (strcmp(userList[i].username,CurrentUsername)==0) && (strcmp(userList[i].password,CurrentPass)==0) ) // khop
          {
                 // change vao database

          	strcpy(userList[i].username,newUsername);
          	strcpy(userList[i].password,newPassword);
          	strcpy(userList[i].fullname,newFullname);

          	logline(LOG_INFO,"Client change accout success.");
          	int n;
          	char msg[] = "CHANGE_SUCCESS/\n";
          	n = write(socket,msg,strlen(msg));
          	if (n < 0)
          	{
          		perror("ERROR Send to socket");
          		logline(LOG_ERROR,"Send failed");
          		exit(1);
          	}

          	return;
          }
      }
  }

    // gui thong bao khong thanh cong
  logline(LOG_ERROR,"Client change accout unsuccess.");
  char msg[] = "LOGIN_FAIL/\n";
  if(send(socket, msg, strlen(msg), 0) < 0)
  {
  	perror("Send failed");
  	logline(LOG_ERROR,"Send failed");
  	exit(1);
  }

  return;
}
void* handle_client_message(void* sock)
{
	int socket = *(int*)sock;
	int read_size;
	char msg[1024];

	bzero((char*)msg,sizeof(msg));
	while((read_size = recv(socket, msg, sizeof(msg), 0)) > 0)
	{
		char fullname[21];
		char username[21];
		char password[21];
		char CurrentUsername[21];
		char CurrentPass[21];
		char ContentMsg[3000];
		char filename[25];
		char ip[16];
		char port[7];

		pid_t  childpid;

		char *type = NULL;

		type = strtok(msg,"/");
		int temp = 0;

		if( strcmp(type, "ONLINE_LIST") ==0 )
		{
			temp = 1;
		}
		if( strcmp(type, "REGISTER") ==0 )
		{
			temp = 2;
            //sscanf(msg,"%*s %s %s %s",fullname, username, password);
			type = strtok(NULL,"/");
			strcpy(fullname, type);
			type = strtok(NULL,"/");
			strcpy(username, type);
			type = strtok(NULL,"/");
			strcpy(password, type);
		}
		if( strcmp(type, "PUBLIC_MESSAGE") ==0 )
		{
			temp = 3;
			type = strtok(NULL,"/");
			strcpy(ContentMsg, type);
		}
		if( strcmp(type, "PRIVATE_MESSAGE") ==0 )
		{
			temp = 4;
            //sscanf(msg,"%*s %s %s", ContentMsg, username);
			type = strtok(NULL,"/");
			strcpy(ContentMsg, type);
			type = strtok(NULL,"/");
			strcpy(username, type);
		}
		if( strcmp(type, "CHANGE_ACCT") ==0 )
		{
			temp = 5;

            //sscanf(msg,"%*s %s %s %s %s %s", CurrentUsername , CurrentPass, fullname, username, password);
			type = strtok(NULL,"/");
			strcpy(CurrentUsername, type);
			type = strtok(NULL,"/");
			strcpy(CurrentPass, type);
			type = strtok(NULL,"/");
			strcpy(fullname, type);
			type = strtok(NULL,"/");
			strcpy(username, type);
			type = strtok(NULL,"/");
			strcpy(password, type);
		}
		if( strcmp(type, "LOGIN") ==0 )
		{
			temp = 6;
            //sscanf(msg,"%*s %s %s", username, password);
			type = strtok(NULL,"/");
			strcpy(username, type);
			type = strtok(NULL,"/");
			strcpy(password, type);
			type = strtok(NULL,"/");
			strcpy(ip, type);
			type = strtok(NULL,"/");
			strcpy(port, type);

		}

		if (strcmp(type, "LOGOUT") == 0)
		{
			temp = 7;
		}

		if (strcmp(type, "SETUPSENDFILE") == 0 )
		{
			temp = 8;
			type = strtok(NULL,"/");
			strcpy(filename, type);
			type = strtok(NULL,"/");
			strcpy(username, type);
		}
		if(strcmp(type, "PORT") == 0)
		{
			temp = 9;
			type = strtok(NULL,"/");
			strcpy(portServerT,type);

		}

		switch(temp)
		{

			case 1:
				send_user_online_list(socket);
				break;

            case 2: // dang ki users
            {               
                childpid = fork();
            	if( childpid == -1)
            	{
            		perror("fork");
            		exit(1);
            	}

            	if(childpid == 0)
            	{
                    // tien trinh con
                    //sem_wait(&sem);     /* P operation */
            		Register(childpid, socket,fullname, username, password);
                    //sem_post(&sem);     /* V operation */

            	}
            	else 
            	{
            		waitpid(-1, NULL, 0);
                    kill(childpid, SIGKILL);

            	}

            	break;
            }

            case 3:
            	send_public_message(socket,ContentMsg);
            	break;

            case 4:
            	send_private_message(socket,ContentMsg, username);
            	break;

            case 5:
                
                // change accout
                if((childpid = fork()) == -1)
                {
                	perror("fork");
                	exit(1);
                }

                if(childpid == 0)
                {
                    // tien trinh con
                    //sem_wait(&sem);     /* P operation */
                	changeAcc( socket,CurrentUsername, CurrentPass, fullname, username, password);
                	
                	//sem_post(&sem);     /* V operation */

                }
                else
                {
                	waitpid(-1, NULL, 0);
                	kill(childpid, SIGKILL);
                }
            	break;

            case 6:
                      // login
	            login(socket,username, password,ip,port);
	            break;

	        case 7:
	        	//logout xong se gui nguoi disconnect
	        	send_user_disconnect(socket);
	        	logout(socket);
	        	printf("Update:\n");
	        	showMenu();
	        	break;

	        case 8:
	        	send_setup_client(socket, username, filename);
	        	send_port_client(socket,username,portServerT);
	        	break;
	        case 9:
	        	printf("Received portServerT: %s \n",portServerT);
	        	break;
            default:
           		fprintf(stderr, "Unknown message type received.\n");
            	break;
        }

    }
    if(read_size == 0)
    {	
    	// gui  cho client biet la client nay da thoat
    	send_user_disconnect(socket);

    	// Cap nhat lai danh sach nguoi online o server
    	int i;
    	for( i = 1; i < MAX_CLIENTS; i++)
    	{
    		if(clients[i].socket == socket)
    		{
    			clients[i].socket = 0;
    			bzero(clients[i].username,sizeof(clients[i].username));
    			bzero(clients[i].ip,sizeof(clients[i].ip));
    			bzero(clients[i].port,sizeof(clients[i].port));
    			break;
    		}

    	}	
    	pthread_exit(NULL);
    	perror("Client disconnected1");
    	fflush(stdout);
    }
    
    else if(read_size == -1)
    {
    	// gui  cho client biet la client nay da thoat
    	send_user_disconnect(socket);

    	// Cap nhat lai danh sach nguoi online o server
    	int i;
    	for( i = 1; i < MAX_CLIENTS; i++)
    	{
    		if(clients[i].socket == socket)
    		{
    			clients[i].socket = 0;
    			bzero(clients[i].username,sizeof(clients[i].username));
    			bzero(clients[i].ip,sizeof(clients[i].ip));
    			bzero(clients[i].port,sizeof(clients[i].port));
    			break;
    		}

    	}
    	pthread_exit(NULL);
    	perror("Client disconnected2");
    }
	
    
    pthread_exit(NULL);  
}

int  Accept_new_connection(connection_info *server_info, connection_info clients[])
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
		if(i == 0 && clients[i].socket == 0)
		{
			clients[0].socket = new_socket;
			socketofServerT = clients[0].socket;
		}
		else
		{
			if(clients[i].socket == 0) {
				clients[i].socket = new_socket;
				break;

			} else if (i == MAX_CLIENTS -1) // if we can accept no more clients
			{
			      //send_too_full_message(new_socket);
			}
		}
	}
	return new_socket;
}

void handle_user_input(connection_info clients[])
{
	char input[255];
	fgets(input, sizeof(input), stdin);
	trim_newline(input);

	if(input[0] == 'q') {
    //stop_server(clients);
	}
}
/************* show online ******************/

void showMenu() 
{
	int i;

	printf("****************************************************************\n");
	
	

	for(i = 1; i < MAX_CLIENTS; i++)
	{
		if(strlen(userList[i].username) != 0 && clients[i].socket != 0 )
		{

			printf("%s\t\t  V\t\t%s\t\t%s\n",userList[i].username,clients[i].ip, clients[i].port );
			


		}
		if(strlen(userList[i].username) != 0 && clients[i].socket == 0)
		{
			printf("%s\n", userList[i].username);

		}
	}
/*
	printf("*******Danh sach Online***********\n");
	for(i = 1; i < MAX_CLIENTS; i++)
	{
		if(clients[i].socket != 0)
		{

			printf("%s\n",clients[i].username );

		}
	}

	printf("*******Danh sach Offline***********\n");
	for(i = 1; i < MAX_CLIENTS; i++)
	{
		if(strlen(userList[i].username) != 0)
		{

			if(clients[i].socket == 0)
			{
				printf("%s\n",userList[i].username );

			}

		}
	}

*/

}
