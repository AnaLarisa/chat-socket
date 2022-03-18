#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "server.h"

#define bufferLength 1400

int server_sockfd = 0, client_sockfd = 0;
ClientList *root, *now;

char *sendBuffer;

// here we sent message to all clients

void sendMsgToAllClients(ClientList *np, char *tmp_buffer)
{
	ClientList *tmp = root->link;

	while(tmp != NULL)
	{
		if(np->data != tmp->data){
			printf("Send to sockfd %d: \"%s\" \n ",tmp->data, tmp_buffer);
			send(tmp->data,tmp_buffer,bufferLength,0);
		}
		tmp = tmp->link;
	}
}

// ctrl+c catch for the server

void catchCtrlCAndExit(int sig)
{
	ClientList *tmp;
	while(root != NULL){
		
		printf("\n Close socketfd: %d\n",root->data);
		//send(tmp->data,"EXIT",7,0);
		close(root->data);
		tmp = root;
		root = root->link;
		free(tmp);
	}
	printf("Closing server chose by the admin (CTRL + C )\n");
	exit(EXIT_SUCCESS);
}

// checking if a user is already on our database

int verifiyExistenceOfUser(char *username)
{
	FILE *fin = fopen("BazeDateUser.txt","r");
	if(fin == NULL)
	{
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	char *buffer = malloc(bufferLength*sizeof(char *));
	if(buffer == NULL)
	{
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	int resultUsername = 0;

	while(fgets(buffer,bufferLength,fin) != NULL){
		buffer[strlen(buffer) - 1] = '\0';
		//printf("%s \n",buffer);
		if(strcmp(buffer,username) == 0){
			resultUsername = 1;
		}
		
		if(resultUsername !=0){
			break;
		}
	}

	fclose(fin);
	free(buffer);
	if(resultUsername == 1){
		return 1;
	}
	return -1;
}

// adding a new user on the database

int insertUserOnDataBase(char *username)
{
	char buffer[bufferLength];
	sprintf(buffer, "%s\n", username);

	FILE *fin = fopen("BazeDateUser.txt","a");
	if(fin == NULL)
	{
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	fputs(buffer,fin);

	fclose(fin);
	return 0;
}

// adding message to the message file

void insertMsgToFile(char *message)
{
	FILE *fin = fopen("message.txt", "a");
	if(fin == NULL)
	{
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	char buffer[bufferLength];
	sprintf(buffer,"%s\n",message);
	fputs(buffer,fin);

	fclose(fin);
}

// here we are handling the client

void clientHandler(void *p_client){
	int leaveFlag = 0;
	char nickname[bufferLength] = {};
	char recvBuffer[bufferLength] = {};


	ClientList *np = (ClientList*) p_client;

	if(recv(np->data, nickname,bufferLength, 0) <= 0 || strlen(nickname) < 2 || strlen(nickname) >= bufferLength - 1){
		printf("%s didn't input name. \n", np->ip);
		leaveFlag = 1;
	}else{
		// verifiy file 
		if(verifiyExistenceOfUser(nickname) > 0)
		{
			printf("The user %s that is trying to join is existing on our database!",nickname);
			strncpy(np->name, nickname, bufferLength);
			printf("%s (%s)(%d) join the chatroom.\n", np->name, np->ip, np->data);
			sprintf(sendBuffer, "%s(%s) join the chatroom.", np->name, np->ip);
			sendMsgToAllClients(np,sendBuffer);
		}else{
			if(insertUserOnDataBase(nickname) == 0){
				printf("we are inserting the user : %s \n", nickname);
			}
			strncpy(np->name, nickname, bufferLength);
			printf("%s (%s)(%d) join the chatroom.\n", np->name, np->ip, np->data);
			sprintf(sendBuffer, "%s(%s) join the chatroom.\n", np->name, np->ip);
			sendMsgToAllClients(np,sendBuffer);
		}
	}

	send(np->data,"message.txt",12,0);

	// start of a conversation

	while(1){
		if(leaveFlag == 1){
			break;
		}
		int receive = recv(np->data, recvBuffer, bufferLength, 0);
		// printf("size : %lu", strlen(recvBuffer));
		if(receive > 0){
			if(strlen(recvBuffer) == 0){
				continue;
			}
			sprintf(sendBuffer, "%s: %s ", np->name, recvBuffer);
			insertMsgToFile(sendBuffer);
		}else if(receive == 0 || strcmp(recvBuffer, "exit") == 0){
			printf("%s(%s)(%d) leave the chatroom", np->name, np->ip, np->data);
			sprintf(sendBuffer, "%s(%s) leave the chatroom.\n", np->name, np->ip);
			leaveFlag = 1;
		}
		sendMsgToAllClients(np,sendBuffer);
		
	}


	close(np->data);
	if(np == now){
		now = np->prev;
		now->link = NULL;
	}else{
		np->prev->link = np->link;
	}
	free(np);
}

int main(int argc, char* argv[])
{
	sendBuffer = malloc(bufferLength * sizeof(char *));
	if(sendBuffer == NULL)
	{
		perror("malloc");
		exit(1);
	}

	//signal(SIGINT, catchCtrlCAndExit);
	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(server_sockfd == -1)
	{
		perror("failed to created socket");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_info, client_info;
	int server_addrlen = sizeof(server_info);
	int client_addrlen = sizeof(client_info);

	memset(&server_info, 0, server_addrlen);
	memset(&client_info, 0, client_addrlen);

	server_info.sin_family = PF_INET;
	server_info.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_info.sin_port = htons(8000);

	bind(server_sockfd, (struct sockaddr *)&server_info, server_addrlen);
	listen(server_sockfd, 5);

	getsockname(server_sockfd, (struct sockaddr*) &server_info, (socklen_t*) & server_addrlen);
	printf("Server is started on : %s : %d \n", inet_ntoa(server_info.sin_addr), ntohs(server_info.sin_port));

	root = newNode(server_sockfd, inet_ntoa(server_info.sin_addr));

	now = root;

	while(1)
	{
		printf("***SERVER ON***\n");

		client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_info, (socklen_t*)&client_addrlen);

		if(client_sockfd == -1){
			perror("could not accept a client");
			exit(EXIT_FAILURE);
		}

		getpeername(client_sockfd, (struct sockaddr*)&client_info, (socklen_t*) &client_addrlen);

		printf("Client %s:%d come in \n", inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));

		ClientList *c = newNode(client_sockfd, inet_ntoa(client_info.sin_addr));
		c->prev = now;

		now->link = c;
		now = c;

		pthread_t id; 
		if(pthread_create(&id, NULL, (void *)clientHandler, (void *)c) !=0){
			perror("thread creeation error");
			exit(EXIT_FAILURE);
		}
	}

	return 0;
}