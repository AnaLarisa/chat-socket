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

int server_sockfd = 0, client_sockfd = 0;
ClientList *root, *now;

void verifiyExistenceOfUser(char *username, char *password)
{
	FILE *fin = fopen("BazeDateUser.txt","r");
	if(fin == NULL)
	{
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	char *buffer = malloc(1024*sizeof(char *));
	if(buffer == NULL)
	{
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	while(fgets(buffer,1024,fin) !=NULL){
		printf("%s ",buffer);
	}
	fclose(fin);
}

void sendMsgToAllClients(ClientList *np, char tmp_buffer[])
{
	ClientList *tmp = root->link;

	while(tmp != NULL)
	{
		if(np->data != tmp->data){
			printf("Send to sockfd %d: \"%s\" \n ",tmp->data, tmp_buffer);
			send(tmp->data,tmp_buffer,1024,0);
		}
		tmp = tmp->link;
	}
}

void clientHandler(void *p_client){
	int leaveFlag = 0;
	char nickname[1024] = {};
	char password[1024] = {};
	char recvBuffer[1024] = {};
	char sendBuffer[1024] = {};

	ClientList *np = (ClientList*) p_client;

	if(recv(np->data, nickname,1024, 0) <= 0 || strlen(nickname) < 2 || strlen(nickname) >= 1024 - 1){
		printf("%s didn't input name. \n", np->ip);
		leaveFlag = 1;
	}
	if(recv(np->data, password,1024, 0) <= 0 || strlen(password) < 2 || strlen(password) >= 1024 - 1){
		printf("%s didn't input password. \n", np->ip);
		leaveFlag = 1;
	}else{
		// verifiy file 
		verifiyExistenceOfUser(nickname,password);
		strncpy(np->password, password,1024);
		strncpy(np->name, nickname, 1024);
		printf("%s %s (%s)(%d) join the chatroom.\n", np->name,np->password, np->ip, np->data);
		sprintf(sendBuffer, "%s(%s) join the chatroom.", np->name, np->ip);
		sendMsgToAllClients(np,sendBuffer);
	}


	// start of a conversation

	while(1){
		if(leaveFlag){
			break;
		}
		int receive = recv(np->data, recvBuffer, 1024, 0);
		if(receive > 0){
			if(strlen(recvBuffer) == 0){
				continue;
			}
			sprintf(sendBuffer, "%s: %s ", np->name, recvBuffer);
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
	server_info.sin_addr.s_addr = INADDR_ANY;
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