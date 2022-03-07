#include <stdio.h>
#include "string.h"

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