#include <cstdio>

#include <signal.h>

static volatile bool keepRunning = true;

void intHandler(int dummy) {
	printf("Handler\n");
	keepRunning = 0;
}

#include <unistd.h>

int main(){
	signal(SIGINT,  intHandler);
	signal(SIGTERM, intHandler);
	signal(SIGHUP,  intHandler);

	while (keepRunning){
		printf(".");
		sleep(1);
	}

	printf("Done\n");
}

