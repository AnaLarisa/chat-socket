#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define bufferLength 1028

int sockfd = 0;
char *username;

int leaving = 0;

// exception of ctrl+c

void catchCtrlCAndExit(int n){
	printf("** leaving the chatroom **\n");
	leaving = 1;
}

void strTrimLf(char *arr, int length){
	int i;
	for(i = 0; i<length; i++){
		if(arr[i] == '\n'){
			arr[i] = '\0';
			break;
		}
	}
}

// add > before all conv

void strOverWriteStdout(){
	printf("\r%s", ">");
	fflush(stdout);
}

// receive message from server to client

void recvMsgHandler()
{
	char receiveMessage[bufferLength] = {};
	while(1){
		int receive = recv(sockfd, receiveMessage, bufferLength, 0);
		if(receive > 0 && strcmp(receiveMessage,"message.txt") !=0){
			printf("\r%s\n", receiveMessage);
			strOverWriteStdout();
		}else if(receive > 0 && strcmp(receiveMessage,"message.txt") ==0){

			pid_t process = fork();
			if(process == -1){
				perror("fork()");
				exit(EXIT_FAILURE);
			}
			if(process == 0){

				if(execlp("cat","cat",receiveMessage,NULL,NULL) == -1){
					perror("execlp");
					exit(EXIT_FAILURE);
				}

				exit(EXIT_SUCCESS);
			}
			int status;
			waitpid(process,&status,0);
			strOverWriteStdout();
		}else{

		}
	}
}

// send message from client to server

void sendMsgHandler(){
	char message[bufferLength] = {};
	while(1){
		strOverWriteStdout();
		while(fgets(message,bufferLength, stdin) != NULL){
			strTrimLf(message,bufferLength);
			if(strlen(message) == 0){
				strOverWriteStdout();
			}else{
				break;
			}
		}
		send(sockfd,message,bufferLength,0);
		if(strcmp(message, "exit") == 0){
			break;
		}
	}
}


int main(int argc, char* argv[])
{	
	signal(SIGINT, catchCtrlCAndExit);

	username = malloc(bufferLength * sizeof(char *));


	printf("enter your username : ");

	if(fgets(username,bufferLength,stdin) != NULL){
		strTrimLf(username,bufferLength);
	}

	pid_t process = fork();
	if(process == -1){
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if(process == 0){
		if(execlp("clear","clear",NULL,NULL) == -1){
			perror("clear");
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS);
	}

	int status;
	if(waitpid(process,&status,0) == -1){
		perror("waitpid");
		exit(EXIT_FAILURE);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1){
		perror("error on creating socket");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_info, client_info;
	int server_addrlen = sizeof(server_info), client_addrlen = (sizeof(client_info));

	memset(&server_info,0,server_addrlen);
	memset(&client_info,0,client_addrlen);
	server_info.sin_family = PF_INET;
	server_info.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_info.sin_port = htons(8000);

	int connectionError = connect(sockfd, (struct sockaddr*)& server_info, server_addrlen);

	if(connectionError == -1)
	{
		perror("connection error on server");
		exit(EXIT_FAILURE);
	}


	getsockname(sockfd, (struct sockaddr*)&client_info, (socklen_t *)&client_addrlen);
	getsockname(sockfd, (struct sockaddr*)&server_info, (socklen_t *)&server_addrlen);

	printf("connect to server: %s:%d \n", inet_ntoa(server_info.sin_addr), ntohs(server_info.sin_port));
	printf("you are: %s:%d\n",inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));

	send(sockfd,username,bufferLength,0);

	pthread_t sendMsgThread;

	if(pthread_create(&sendMsgThread, NULL, (void*) sendMsgHandler, NULL) != 0){
		perror("error on creating threads");
		exit(EXIT_FAILURE);
	}

	pthread_t recvMsgThread;
	if(pthread_create(&recvMsgThread, NULL, (void*) recvMsgHandler, NULL) !=0){
		perror("error on creating threads");
		exit(EXIT_FAILURE);
	}

	while(1){
		if(leaving == 1){
			break;
		}
	}
	free(username);
	close(sockfd);
	return 0;
}