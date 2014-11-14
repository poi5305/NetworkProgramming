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