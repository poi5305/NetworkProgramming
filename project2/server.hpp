#ifndef __SERVER__
#define __SERVER__

#include <sys/socket.h> // socket
#include <netinet/in.h> // sockaddr_in
#include <netdb.h> // getservbyname
#include <stdlib.h> // atoi
#include <string>
#include "console.hpp"
#include "shell.hpp"

class server
{
public:
	server(std::function<void()> server_function)
	{
		signal(SIGCHLD, [](int k){});
		server_function();
	}
	server(int address, int port, std::function<void(int)> server_function)
	{
		int server_post = port;
		int sockfd, newsockfd, clilen, child_pid;
		struct sockaddr_in server_addr, client_addr;
		std::vector<int> child_pids(0);
		
		if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
			console::error("Error, can't not open socket.");
		
		memset((void*) &server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = htonl(address);
		server_addr.sin_port = htons(server_post);
		
		if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
		{
			std::cout << "Error, can't bind socket" << std::endl;
			//console::error("Error, can't bind socket");
			server_addr.sin_port = htons(server_post+1);
			bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
			std::cout << server_post+1 << std::endl;
		}
		listen(sockfd, 1);
		
		for(int i=0; ;i++)
		{
			newsockfd = accept(sockfd, (struct sockaddr *) &client_addr, (socklen_t *) &clilen);
			//std::cout << client_addr.sin_addr.s_addr << std::endl;
			if (newsockfd < 0)
				console::error("Error, can't accept");
			if ( (child_pid = fork()) < 0  )
				console::error("Error, can't fork");
			else if (child_pid == 0)
			{// child procress
				close(sockfd);
				signal(SIGCHLD, [](int k){});
				//signal(SIGCHLD, nothinghandler); // child not auto wait
				server_function(newsockfd);
				exit(0);
			}
			else
			{// parent procress
				child_pids.push_back(child_pid);
			}
			close(newsockfd);
		}
		close(sockfd);
	}
	
	
	static void multiple_process_server(std::string service, std::function<void(int)> server_function)
	{
		//std::cerr << "multiple process server" << std::endl;
		int master_socket_fd = server::passivsock(service);
		if(master_socket_fd == -1)
			master_socket_fd = server::passivsock(std::to_string(atoi( service.c_str() )+1));
			
		int newsockfd, clilen, child_pid;
		struct sockaddr_in client_addr;
		memset((void*) &client_addr, 0, sizeof(sockaddr_in));
		
		std::vector<int> child_pids(0);
		
		for(int i=0; ;i++)
		{
			newsockfd = accept(master_socket_fd, (struct sockaddr *) &client_addr, (socklen_t *) &clilen);
			//std::cout << client_addr.sin_addr.s_addr << std::endl;
			if (newsockfd < 0)
				console::error("Error, can't accept");
			if ( (child_pid = fork()) < 0  )
				console::error("Error, can't fork");
			else if (child_pid == 0)
			{// child procress
				console::debug("New client create");
				close(master_socket_fd);
				signal(SIGCHLD, [](int k){});
				
				char ip[INET_ADDRSTRLEN];
				inet_ntop( AF_INET, &(client_addr.sin_addr.s_addr), ip, INET_ADDRSTRLEN );
				uint16_t port = client_addr.sin_port;
				//int sockfd, int pid, std::string sockname, std::string ip, std::string port
				shell<multiple_process> my_sh(newsockfd, child_pid, "(no name)", ip, std::to_string(port));
				
				exit(0);
			}
			else
			{// parent procress
				child_pids.push_back(child_pid);
				
			}
			close(newsockfd);
		}
		close(master_socket_fd);
	}
	
	static void single_process_server(std::string service, std::function<void(int)> server_function)
	{
		//std::cerr << "single process server" << std::endl;
		int master_socket_fd = server::passivsock(service);
		if(master_socket_fd == -1)
			master_socket_fd = server::passivsock(std::to_string(atoi( service.c_str() )+1));
		
		int newsockfd, clilen, child_pid;
		struct sockaddr_in client_addr;
		memset((void*) &client_addr, 0, sizeof(sockaddr_in));
		
		fd_set read_fds; /* read file descriptor set */
		fd_set active_fds; /* active file descriptor set */
		int number_fds;
		
		number_fds = getdtablesize();
		if(number_fds > 64)
			number_fds = 64;
		
		FD_ZERO(&active_fds);
		FD_SET(master_socket_fd, &active_fds);
	
		std::map<int, shell<single_process>> shell_sets;
	
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
					int clilen;
					int new_socket = accept(master_socket_fd, (struct sockaddr *) &client_addr, (socklen_t *) &clilen);
					
					//std::cerr << "new_socket " << new_socket << std::endl;
					if(new_socket < 0)
						console::error("Can't accept");
					FD_SET(new_socket, &active_fds);
					
					char ip[INET_ADDRSTRLEN];
					inet_ntop( AF_INET, &(client_addr.sin_addr.s_addr), ip, INET_ADDRSTRLEN );
					uint16_t port = client_addr.sin_port;
					shell_sets.insert({new_socket, shell<single_process>(new_socket, "(no name)", ip, std::to_string(port)) });
					
				}
				else
				{ // this fd is active
					console::debug("Old client active");
					//std::cerr << "fd" << fd << " -> " << shell_sets[fd].socket_fd << std::endl;
					int status = shell_sets[fd].run_one_time();
					
					if(status == 0)
					{
						console::debug("Close client");
						shell_sets.erase(fd);
						close(fd);
						close(0);dup(master_socket_fd);
						close(1);dup(master_socket_fd);
						close(2);dup(master_socket_fd);
						FD_CLR(fd, &active_fds);
						
					}
				}
			}
		}
	}
	
	
	static int passivsock(std::string service, std::string protocol = "tcp")
	{
		struct servent *p_server_info;
		struct protoent *p_protocol_info;
		struct sockaddr_in sin;
		int socket_fd, socket_type;
		
		memset((void*) &sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = INADDR_ANY;
		
		///@brief set service (port number)
		p_server_info = getservbyname(service.c_str(), protocol.c_str());
		if( p_server_info )
			sin.sin_port = htons( ntohs( (u_short)(p_server_info->s_port) ) );
		else
			sin.sin_port = htons( (u_short)( atoi(service.c_str()) ) );
		if(sin.sin_port == 0)
			console::error("Can't get service entry");
		
		///@brief set protocol (udp or tcp)
		p_protocol_info = getprotobyname(protocol.c_str());
		if(p_protocol_info == 0)
			console::error("Can't get protocol entry");
		if(protocol == "udp")
			socket_type = SOCK_DGRAM;
		else
			socket_type = SOCK_STREAM;
		
		///@brief allocate a socket
		socket_fd = socket(PF_INET, socket_type, p_protocol_info->p_proto);
		if(socket_fd < 0)
			console::error("Can't create socket");
		if( bind(socket_fd, (struct sockaddr*)&sin, sizeof(sin)) < 0)
			return -1;//console::error("Can't bind socket");
		if(socket_type == SOCK_STREAM)
			if(listen(socket_fd, 0)<0)
				console::error("Can't listen socket");
		return socket_fd;
	}
	
	
};

#endif
