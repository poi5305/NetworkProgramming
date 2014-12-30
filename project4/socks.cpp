#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/uio.h>

#include <string>
#include <vector>
#include <map>
#include <fstream>

#include <iostream>
#include <functional>
#include "server.hpp"

struct REQUEST_INFO
{
	uint8_t vn;
	uint8_t cd;
	uint16_t port;
	uint32_t ip;
	// 8 bytes
	std::string user_id;
	std::string domain_name;
};

struct Client
{
	int sockfd;
	bool is_connected;
	uint16_t port;
	uint32_t ip;
	struct sockaddr_in server_addr;
};

class SOCKS
{
	int s_sockfd;
	int c_sockfd;
	int b_sockfd;
	REQUEST_INFO request_info;
	Client client;
	
	fd_set wfds, rfds, afds;
	
public:
	
	SOCKS(int fd)
		:s_sockfd(fd)
	{
		std::cerr << "NEW CLIENT" << std::endl;
		FD_ZERO(&rfds);
		FD_ZERO(&afds);
		init();
	}
	void init()
	{
		std::cerr << "sockfd " << s_sockfd << " "<< sizeof(long) << std::endl;
		get_request();
		int state = create_client();
		
		FD_SET(c_sockfd, &afds);
		FD_SET(s_sockfd, &afds);
		
		if(state == -1)
		{
			send_request(false);
			std::cerr << "client fail" << std::endl;
		}
		else
		{
			if(request_info.cd == 2)
			{
				b_sockfd = server::passivsock(std::to_string(client.port+1));
				std::cerr << "bind sockfd:" << b_sockfd << std::endl;
			}
			send_request(true);
			std::cerr << "client success" << std::endl;
		}
		std::cerr << "state " << state << std::endl;
		
		
		run();
		
	}
	void close_all()
	{
		FD_CLR(c_sockfd, &afds);
		FD_CLR(s_sockfd, &afds);
		close(c_sockfd);
		close(s_sockfd);
		exit(0);
	}
	void run()
	{
		int nfds = c_sockfd+5;
		
		char s_buf[4097];
		char c_buf[4097];
		
		struct timeval ts;
		ts.tv_sec = 6;
		ts.tv_usec = 0;
		
		int retry = 0;
		int select_state = 0;
		while(true)
		{
			memcpy(&rfds, &afds, sizeof(fd_set));
			//std::cerr << "select wait " << std::endl;
			
			if((select_state = select(nfds, &rfds, NULL, NULL, &ts)) < 0)
			{
				std::cerr << errno << "\n";
				perror("cannot select");
			}
			//std::cerr << "select wait finish "<< select_state << FD_ISSET(c_sockfd, &rfds)<< FD_ISSET(s_sockfd, &rfds) << std::endl;
			
			if(select_state == 0)
			{
				std::cerr << "========== Time OUT ==========" << std::endl;
				close_all();
			}
			
			// client
			if (FD_ISSET(c_sockfd, &rfds))
			{
				//std::cerr << "B" << std::endl;
				int len, len2;
				
				len = read(c_sockfd, c_buf, 4096);
				len2 = write(s_sockfd, c_buf, len);
				
				if(len == 0)
				{
					//std::cerr << "CLOSE" << std::endl;
					close_all();
				}
			}
			
			if (FD_ISSET(s_sockfd, &rfds))
			{
				int len = read(s_sockfd, s_buf, 4096);
				int len2 = write(c_sockfd, s_buf, len);
				if(len == 0)
				{
					//std::cerr << "inactive ssockfd" << std::endl;
					//FD_CLR(s_sockfd, &afds);
					close_all();
				}
				//std::cerr << "BB len len2 " << len << " " << len2 << std::endl;
			}
		}
	}	
	void parse_request(char *buf, int len)
	{
		if(len == 9)
		{
			request_info.vn = buf[0];
			request_info.cd = buf[1];
			request_info.port = ((uint8_t)buf[2]) << 8 | (uint8_t)buf[3];
			request_info.ip = (((uint8_t)buf[4])<<24 | ((uint8_t)buf[5])<<16 | ((uint8_t)buf[6])<<8 | (uint8_t)buf[7]);
			
			std::cout << "request_info.vn: " << (int) request_info.vn << std::endl;
			std::cout << "request_info.cd: " << (int) request_info.cd << std::endl;
			std::cout << "request_info.port " << (int) request_info.port << std::endl;
			std::cout << "request_info.ip: " <<  request_info.ip << std::endl;
		}
		if(len > 9)
		{
			request_info.user_id = (buf+8);
			std::cout << "request_info.user_id: " << request_info.user_id << std::endl;
		}
		if(len > (request_info.user_id.size()+10) )
		{
			request_info.domain_name = (buf+(request_info.user_id.size()+8+1));
			std::cout << "request_info.domain_name: " << request_info.domain_name << std::endl;
		}
	}
	
	void get_request()
	{
		char buf[256];
		int len = 0;
		int len2 = 0;
		len = read(s_sockfd, buf, 256);
		for(int i=0; i<len; i++)
			std::cerr << "buf " << (int) buf[i] << std::endl;

		parse_request(buf, len);
		std::cerr << "fffget_request len " << len << " len2 " << len2 << std::endl;
	}
	void send_request(bool success)
	{
		char pack[8];
		
		pack[0] = 0;
		pack[1] = (uint8_t)90;
		pack[2] = client.port/256;
		pack[3] = client.port%256;
		pack[4] = client.ip >> 24;
		pack[5] = (client.ip>>16) & 0xFF;
		pack[6] = (client.ip>8) & 0xFF;
		pack[7] = client.ip & 0xFF;
		
		if(!success)
			pack[1] = (uint8_t)91;
		std::cerr << (int) pack[1] << std::endl;
		int len = write(s_sockfd, pack, 8);
		std::cerr << "send r " << len << std::endl;
	}
	int create_client()
	{
		int sockfd = socket(AF_INET, SOCK_STREAM, 0);
		c_sockfd = sockfd;
		
		client.sockfd = sockfd;
		client.ip = request_info.ip;
		client.port = request_info.port;
		client.is_connected = false;
		
		memset(&client.server_addr, '0', sizeof(client.server_addr));
		client.server_addr.sin_family = AF_INET;
		client.server_addr.sin_addr.s_addr = htonl(request_info.ip);
		client.server_addr.sin_port = htons(client.port);
		
		print_ip(client.server_addr.sin_addr);
		
		//int flags = fcntl(client.sockfd, F_GETFL, 0);
		//fcntl(client.sockfd, F_SETFL, flags | O_NONBLOCK);
		
		int state = connect(client.sockfd, (sockaddr *) &client.server_addr, sizeof(client.server_addr));
		
		
		
		/*
		int state = 0;
		int times = 50;
		while(true)
		{
			times--;
			usleep(100000);
			state = connect(client.sockfd, (sockaddr *) &client.server_addr, sizeof(client.server_addr));
			if(state == 0)
				return 0;
			if(times == 0)
				break;
		}
		*/
		return state;
	}
	
	void print_ip(in_addr &addr)
	{
		char *some_addr;
		some_addr = inet_ntoa(addr);
		printf("%s\n", some_addr); // prints "10.0.0.1"
	}
};

int main(int argc, char** argv)
{
	signal(SIGUSR1, [](int k){});
	signal(SIGUSR2, [](int k){});
	signal(SIGCHLD, SIG_IGN); // signal(SIGCHLD, childhandler);
	std::function<void(server_info)> run_socks = [](server_info s_info)
	{
		//HTTPD httpd(s_info.sockfd);
		SOCKS(s_info.sockfd);
	};
	
	
	server::forked_server(argv[1], run_socks);
	
	return 0;
}