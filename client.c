#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

int sockfd = 0;
char *username;
char *password;

int leaving = 0;

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

void strOverWriteStdout(){
	printf("\r%s", ">");
	fflush(stdout);
}

void recvMsgHandler()
{
	char receiveMessage[1024] = {};
	while(1){
		int receive = recv(sockfd, receiveMessage, 1024, 0);
		if(receive > 0){
			printf("\r%s\n", receiveMessage);
			strOverWriteStdout();
		}else if(receive == 0){
			break;
		}else{

		}
	}
}

void sendMsgHandler(){
	char message[1024] = {};
	while(1){
		strOverWriteStdout();
		while(fgets(message,1024, stdin) != NULL){
			strTrimLf(message,1024);
			if(strlen(message) == 0){
				strOverWriteStdout();
			}else{
				break;
			}
		}
		send(sockfd,message,1024,0);
		if(strcmp(message, "exit") == 0){
			break;
		}
	}
}


int main(int argc, char* argv[])
{	
	signal(SIGINT, catchCtrlCAndExit);

	username = malloc(1024 * sizeof(char *));
	password = malloc(1024 * sizeof(char *));


	printf("enter you're username : ");

	if(fgets(username,1024,stdin) != NULL){
		strTrimLf(username,1024);
	}

	printf("enter you're password : ");

	if(fgets(password,1024,stdin) != NULL){
		strTrimLf(password,1024);
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

	send(sockfd,username,1024,0);
	send(sockfd,password,1024,0);

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

	close(sockfd);
	return 0;
}