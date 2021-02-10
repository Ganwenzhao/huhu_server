#include "../include/HttpServer.h"

int main(int argc, char** argv){
	int port = 6666;
	if(argc >= 2) {
		port = atoi(argv[1]);
	}
	
	int thread_num = 4;
	if(argc >= 3) {
		thread_num = atoi(argv[2]);
	}

	huhu::HttpServer server(port, thread_num);
	server.runHuHu();

	return 0;
}
