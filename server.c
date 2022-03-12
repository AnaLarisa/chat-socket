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

void catchCtrlCAndExit(int sig)
{
	ClientList *tmp;
	while(root != NULL){
		printf("\n Close socketfd: %d\n",root->data);
		close(root->data);
		tmp = root;
		root = root->link;
		free(tmp);
	}
	printf("Closing server chose by the admin (CTRL + C )\n");
	exit(EXIT_SUCCESS);
}

int checkIfUserAlreadyExistInFile(char *username)
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

	int result = 0;

	while(fgets(buffer,1024,fin) != NULL){
		buffer[strlen(buffer) - 1] = '\0';
		if(strcmp(buffer,username) == 0){
			result = 1;
		}
		fgets(buffer,1024,fin);
		buffer[strlen(buffer) - 1] = '\0';
		if(result!=0){
			break;
		}
	}

	fclose(fin);
	free(buffer);

	return result;

}

int verifiyExistenceOfUser(char *username, char *password)
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

	int resultUsername = 0, resultPassword = 0;

	while(fgets(buffer,1024,fin) != NULL){
		buffer[strlen(buffer) - 1] = '\0';
		//printf("%s \n",buffer);
		if(strcmp(buffer,username) == 0){
			resultUsername = 1;
		}
		fgets(buffer,1024,fin);
		buffer[strlen(buffer) - 1] = '\0';
		//printf("%s \n",buffer);
		if(strcmp(buffer,password) ==0){
			resultPassword = 1;
		}
		if(resultPassword != 0 && resultUsername !=0){
			break;
		}
	}

	fclose(fin);
	free(buffer);
	if(resultUsername == 1 && resultPassword == 1){
		return 1;
	}
	return -1;
}

int insertUserOnDataBase(char *username, char *password)
{
	char buffer[1024];
	sprintf(buffer, "%s\n%s\n", username, password);

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
		if(verifiyExistenceOfUser(nickname,password) > 0)
		{
			printf("The user %s that is trying to join is existing on our database!",nickname);
			strncpy(np->password, password,1024);
			strncpy(np->name, nickname, 1024);
			printf("%s %s (%s)(%d) join the chatroom.\n", np->name,np->password, np->ip, np->data);
			sprintf(sendBuffer, "%s(%s) join the chatroom.", np->name, np->ip);
			sendMsgToAllClients(np,sendBuffer);
		}else{
			if(checkIfUserAlreadyExistInFile(nickname) == 1){
				printf("username %s already exist... not entering the chat.. ", nickname);
				leaveFlag = 1;
			}else{
				if(insertUserOnDataBase(nickname,password) == 0){
					printf("we insert the user : %s \n", nickname);
				}
				strncpy(np->password, password,1024);
				strncpy(np->name, nickname, 1024);
				printf("%s %s (%s)(%d) join the chatroom.\n", np->name,np->password, np->ip, np->data);
				sprintf(sendBuffer, "%s(%s) join the chatroom.", np->name, np->ip);
				sendMsgToAllClients(np,sendBuffer);
			}
		}
	}


	// start of a conversation

	while(1){
		if(leaveFlag == 1){
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
	signal(SIGINT, catchCtrlCAndExit);
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