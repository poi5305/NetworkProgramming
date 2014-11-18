#include <sys/socket.h> // socket
#include <sys/wait.h> // wait
#include <sys/stat.h> // open flag
#include <netinet/in.h> // sockaddr_in
#include <unistd.h> //exec, close
#include <fcntl.h> // open file
#include <signal.h> //signal
#include <stdlib.h> // getenv setenv
#include <sys/select.h> // select
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include <sys/time.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
//#include <boost/algorithm/string.hpp>
#include "console.hpp"
#include "struct.hpp"

#include "server.hpp"

int main (int argc, char** argv)
{
	signal(SIGUSR1, [](int k){});
	signal(SIGUSR2, [](int k){});
	signal(SIGCHLD, SIG_IGN); // signal(SIGCHLD, childhandler);
	
	struct_utility::shm_init();
	
	users = p_shared->users;
	chart_fifo = p_shared->chart_fifo;
	
	if(argc > 2)
		server::multiple_process_server(argv[1], [](int k){});
	else if(argc > 1)
		server::single_process_server(argv[1], [](int k){});
	else
		shell<multiple_process> my_sh(0, 1, 2);
}