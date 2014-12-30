#ifndef __SERVER__
#define __SERVER__

#include <sys/socket.h> // socket
#include <netinet/in.h> // sockaddr_in
#include <netdb.h> // getservbyname
#include <stdlib.h> // atoi
#include <arpa/inet.h>
#include <string>
#include "console.hpp"


struct server_info
{
	int sockfd;	
	std::string ip;

	int port;
	std::string name;
};

class server
{
public:
	server(std::function<void()> server_function)
	{
		signal(SIGCHLD, [](int k){});
		server_function();
	}
	
	static void forked_server(std::string service, std::function<void(server_info)> server_function)
	{
		std::cerr << "multiple process server" << std::endl;
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
				signal(SIGINT, [](int k){});
				
				char ip[INET_ADDRSTRLEN];
				inet_ntop( AF_INET, &(client_addr.sin_addr.s_addr), ip, INET_ADDRSTRLEN );
				uint16_t port = client_addr.sin_port;
				
				server_info s_info {newsockfd, ip, port, "(no name)"};
				server_function(s_info);
				
				//int sockfd, int pid, std::string sockname, std::string ip, std::string port
				//shell<multiple_process> my_sh(newsockfd, child_pid, "(no name)", ip, std::to_string(port));
				//CGILAB/511
				//shell<multiple_process> my_sh(newsockfd, child_pid, "(no name)", "CGILAB", "511");
				exit(0);
			}
			else
			{// parent procress
				child_pids.push_back(child_pid);
				//wait(0);
			}
			close(newsockfd);
		}
		close(master_socket_fd);
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
