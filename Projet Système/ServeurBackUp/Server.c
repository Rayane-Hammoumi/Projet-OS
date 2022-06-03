#include <stdio.h>
#include <stdlib.h>

void close_server();

int main(){
	atexit(close_server);
	FILE* fServer = NULL;
	fServer = fopen("isOn.txt", "w+");
	fprintf(fServer, "%d", 1);
	fclose(fServer);
	
	int test;
	printf("ceci est un test : ");
	scanf(" %d", &test);
	
	return 0;
}

void close_server(){
	FILE* fServer = NULL;
	fServer = fopen("isOn.txt", "w+");
	fprintf(fServer, "%d", 0);
	fclose(fServer);
}
