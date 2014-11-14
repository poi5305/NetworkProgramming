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

#include <sys/time.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
//#include <boost/algorithm/string.hpp>
#include "server.hpp"




int main (int argc, char** argv)
{
	signal(SIGCHLD, SIG_IGN); // signal(SIGCHLD, childhandler);
	
	std::string service = "5566"; /* service name or port number */
	
	struct sockaddr_in from_cin; /* the from address of a client*/
	int master_socket_fd; /* master server socket */
	fd_set read_fds; /* read file descriptor set */
	fd_set active_fds; /* active file descriptor set */
	//int alen; /* from-address length */
	int number_fds;
	
	master_socket_fd = server::passivsock(service);
	if(master_socket_fd == -1)
		master_socket_fd = server::passivsock("5567");
	number_fds = getdtablesize();
	if(number_fds > 512)
		number_fds = 512;
	
	FD_ZERO(&active_fds);
	FD_SET(master_socket_fd, &active_fds);

	std::map<int, shell> shell_sets;

	while(1)
	{		
		memcpy(&read_fds, &active_fds, sizeof(read_fds)); /* copy active to read */
		int state = select(number_fds, &read_fds, (fd_set *)0, (fd_set *)0, (struct timeval *)0 );	
		if(state < 0)
		{
			printf("Error %s\n", strerror(errno));
			//std::cerr << errno << " "<<EBADF << " " << EFAULT <<std::endl;
			console::error("Select Error");
		}
			
		for(int fd(0); fd < number_fds; ++fd)
		{

			if(!FD_ISSET(fd, &read_fds))
				continue;
			//std::cerr << "active " << fd << std::endl;
			if(fd == master_socket_fd)
			{
				console::debug("New client create");
				int alen;
				int new_socket = accept(master_socket_fd, (struct sockaddr *) &from_cin, (socklen_t *) &alen);
				
				//std::cerr << "new_socket " << new_socket << std::endl;
				if(new_socket < 0)
					console::error("Can't accept");
				FD_SET(new_socket, &active_fds);
				shell_sets.insert({new_socket, shell(new_socket, "Unknow") });
				
				
				char ip[INET_ADDRSTRLEN];
				inet_ntop( AF_INET, &(from_cin.sin_addr.s_addr), ip, INET_ADDRSTRLEN );
				uint16_t port = from_cin.sin_port;
				
				shell_sets[new_socket].update_ip_port(ip, std::to_string(port));
				//shell_sets[new_socket].users[shell_sets[new_socket].user_id].ip = ip;
				//shell_sets[new_socket].users[shell_sets[new_socket].user_id].port = std::to_string(port);
				
			}
			else
			{ // this fd is active
				console::debug("Old client active");
				int status = shell_sets[fd].run_one_time();
				if(status == 0)
				{
					console::debug("Close client");
					shell_sets.erase(fd);
					close(fd);
					close(0);dup(master_socket_fd);
					close(1);dup(master_socket_fd);
					//close(2);dup(master_socket_fd);
					FD_CLR(fd, &active_fds);
					
				}
			}
		}
	}
	return 0;
}
