#include <iostream>

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

#include <boost/algorithm/string.hpp>


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
	int id;
	int sockfd;
	int port;
	int txt_idx;
	uint16_t socks_port;
	uint32_t socks_ip;
	bool is_connected;
	bool is_write;
	int is_socks_ok;
	int is_exit;
	std::string ip;
	std::string txt;
	std::vector<std::string> txt_content;
	struct sockaddr_in server_addr;
	struct hostent *server;
};

class cgi
{
public:	
	std::map<std::string, std::string> query_strings;
	std::vector<Client> clients;
	fd_set wfds, rfds, afds;
	int max_fd;
	REQUEST_INFO request_info;
	cgi()
		:max_fd(2)
	{
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_ZERO(&afds);
	}
	
	int new_client(int id, std::string ip, int port, std::string txt, Client *p_client = NULL)
	{
		if(p_client == NULL)
		{
			clients.emplace_back(Client());
			p_client = &(clients.back());
		}
		auto &client = *p_client;
		
		int sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
		{
			std::cerr << "Cannot open socket.\n";
			exit(1);
		}
		client.id = id;
		client.sockfd = sockfd;
		client.ip = ip;
		client.is_connected = false;
		client.is_write = false;
		client.port = port;
		client.socks_port = 0;
		client.socks_ip = 0;
		client.txt_idx = 0;
		client.txt = txt;
		client.is_socks_ok = 2;
		
		client.server = gethostbyname(client.ip.c_str());
		if (client.server == NULL) {
			std::cerr << "Cannot get host name.\n";
			exit(1);
		}

		memset(&client.server_addr, '0', sizeof(client.server_addr));
		client.server_addr.sin_family = AF_INET;
		client.server_addr.sin_addr = *((struct in_addr *)client.server->h_addr);
		client.server_addr.sin_port = htons(client.port);
		
		// Non blocking
		int flags = fcntl(sockfd, F_GETFL, 0);
		fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
		
		int state = connect(client.sockfd, (sockaddr *) &client.server_addr, sizeof(client.server_addr));
		
		//std::cerr << "state: " << state << " fd: "<< client.sockfd << " Connect: " << client.ip << ":" << client.port << " " << client.txt << " " << client.server->h_addr << std::endl;
		return client.sockfd;
	}
	
	
	void init()
	{
		get_query_string();
		
		for(int i=1;i<=5;i++)
		{
			char ci = i+48;
			std::string idx(&ci,1);
			std::string s_ip = "h" + idx;
			std::string s_port = "p" + idx;
			std::string s_txt = "f" + idx;
			std::string s_sh = "sh" + idx;
			std::string s_sp = "sp" + idx;
			
			int sockfd = 0;
			
			if(query_strings[s_ip] == "" || query_strings[s_port]=="" || query_strings[s_txt] == "")
				continue;
			if(query_strings[s_sh] != "" && query_strings[s_sp] != "")
			{
				sockfd = new_client(i-1, query_strings[s_sh], std::atoi(query_strings[s_sp].c_str()), query_strings[s_txt], NULL);
				
				prepare_socks_info(clients.back(), query_strings[s_ip], query_strings[s_port]);
			}
			else
			{
				sockfd = new_client(i-1, query_strings[s_ip], std::atoi(query_strings[s_port].c_str()), query_strings[s_txt], NULL);
			}
			
			max_fd = std::max(max_fd, sockfd);
			FD_SET(sockfd, &afds);
		}
		
	}
	bool read_txt(Client &client)
	{
		std::string line;
		if( access( client.txt.c_str(), F_OK ) == -1)
		{
			return false;
		}
		std::ifstream fp(client.txt);
		while(std::getline(fp, line))
		{
			client.txt_content.push_back(line+'\n');
		}
		fp.close();
		return true;
	}
	int contain_prompt ( char* line )
	{
		int i, prompt = 0 ;
		for (i=0; line[i]; ++i)
		{
			switch ( line[i] ) {
				case '%' : prompt = 1 ; break;
				case ' ' : if ( prompt ) return 1;
				default: prompt = 0;
			}
		}
		return 0;
	} 
	
	int recv_msg(Client &client)
	{
		char buf[3000],*tmp;
		int len,i;
		
		len=read(client.sockfd, buf,sizeof(buf)-1);
		if(len < 0) return -1;
		
		buf[len] = 0;
		if(len>0)
		{
			for(tmp=strtok(buf,"\n"); tmp; tmp=strtok(NULL,"\n"))
			{
				if ( contain_prompt(tmp) )
					client.is_write = true;
				print_html(client, tmp, false);
				//printf("%d| %s\n",0,tmp);  // echo input
			}
		}
		fflush(stdout); 
		return len;
	}
	bool send_msg(Client &client)
	{
		bool is_file = true;
		if(client.txt_content.size() == 0)
			is_file = read_txt(client);
		if(!is_file)
		{
			print_html(client, "File not exist!!", true);
			client.is_exit = 2;
			return false;
		}
		if(client.is_write && client.txt_idx != client.txt_content.size())
		{
			auto &line = client.txt_content[client.txt_idx];
			//std::cout << line << std::endl;
			print_html(client, line, true);
			std::cerr << line.size() << std::endl;
			write(client.sockfd, line.c_str(), line.size());
			//usleep(100000);
			client.is_write = false;

			if(line.find("exit") != std::string::npos)
			client.is_exit = true;
			client.txt_idx++;
		}
		return true;
	}
	void print_html(Client &client, std::string str, bool border)
	{
		boost::replace_all(str, "\r", " ");
		boost::replace_all(str, "\n", " ");
		std::cout << "<script>document.all['m" << client.id <<"'].innerHTML += \"";
		if (border) std::cout << "<b>";
		std::cout << str;
		if (border) std::cout << "</b>";

		if (str[0] != '%')
			std::cout << "<br/>";
		std::cout << "\";</script>" << std::endl;
		std::cout.flush();
	}
	
	
	
	void run()
	{
		int nfds = max_fd+1;
		//int nfds = getdtablesize();
		//nfds = 8;
		while (true)
		{
			memcpy(&rfds, &afds, sizeof(fd_set));
			memcpy(&wfds, &afds, sizeof(fd_set));
			//if(select(nfds, &rfds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 0)
			//std::cerr << "AAAA "<< std::endl;
			if(select(nfds, &rfds, &wfds, NULL, NULL) < 0)
			{
				std::cerr << errno << "\n";
				perror("cannot select");
			}
			//std::cerr << "BBB "<< std::endl;
			for (auto &client : clients)
			{
				auto client_fd = client.sockfd;
				
				if (FD_ISSET(client_fd, &wfds))
				{
					if(!client.is_connected)
					{
						int state = connect(client.sockfd, (sockaddr *) &client.server_addr, (socklen_t)sizeof(client.server_addr));
						if(state == 0)
							client.is_connected = true;
						else if(state < 0)
						{
							if(errno == EISCONN)
								client.is_connected = true;
							else if(errno == EINVAL)
							{ // bsd Invalid argument
								perror("cannot connect");
								close(client_fd);
								FD_CLR(client_fd, &afds);
								int sockfd = new_client(client.id, client.ip, client.port, client.txt, &client);
								max_fd = std::max(max_fd, sockfd);
								FD_SET(sockfd, &afds);
							}
							else if(errno == ECONNREFUSED)
							{
								perror("cannot connect");
								print_html(client, "Server Not exist! Connect Refused", true);
								client.is_exit = true;
								FD_CLR(client_fd, &afds);
								close(client_fd);
							}
						}
					}
					if(client.is_connected && client.is_socks_ok == 0)
					{ // send socks request
						send_request(client);
						client.is_socks_ok = 1;
					}
				}
				
				if (FD_ISSET(client_fd, &rfds) && client.is_socks_ok)
				{
					//std::cerr << "client.is_socks_ok A " <<  client.is_socks_ok <<std::endl;
					if(client.is_socks_ok == 2)
					{
						// already connect
						recv_msg(client);
						//std::cerr << "send~" << client_fd << std::endl;
						send_msg(client);
						//std::cerr << "send end~" << client_fd << std::endl;
						if(client.is_exit == 2)
						{
							FD_CLR(client_fd, &afds);
							close(client_fd);
						}
					}
					else
					{
						get_reply(client);
						if(request_info.cd == 90)
						{		
							client.is_socks_ok = 2;
							//std::cerr << "client.is_socks_ok B " <<  client.is_socks_ok <<std::endl;
						}
						else
						{
							//std::cerr << "client.is_socks_ok C " <<  client.is_socks_ok <<std::endl;
							client.is_exit = 2;
							FD_CLR(client_fd, &afds);
							close(client_fd);
						}
					}
					
				}
			}
			
			// check is all client finish
			bool all_finish = true;
			for (auto &client : clients)
			{
				//std::cerr << "check " << client.sockfd << " " << client.is_exit << std::endl;
				if(client.is_exit != 2)
					all_finish = false;
				if(client.is_exit == 1)
					client.is_exit = 2;
			}
			if(all_finish)
			{
				break;
			}	
			//usleep(100000);
		}
	}
	
	void prepare_socks_info(Client& client, std::string ip, std::string port)
	{
		client.is_socks_ok = false;
		
		struct hostent* server;	
		
		server = gethostbyname(ip.c_str());
		if (server == NULL) {
			std::cerr << "Cannot get host name.\n";
			exit(1);
		}
		
		in_addr addr = *( (struct in_addr *)client.server->h_addr );

		//uint16_t socks_port;
		//uint32_t socks_ip;
		client.socks_port = atoi(port.c_str());
		client.socks_ip = htonl(addr.s_addr);
		std::cerr << "socks_port " << client.socks_port << " addr.s_addr " << client.socks_ip << " addr.s_addr " << addr.s_addr << std::endl;
	}
	void parse_request(char *buf, int len)
	{
		if(len == 8)
		{
			request_info.vn = buf[0];
			request_info.cd = buf[1];
			request_info.port = ((uint8_t)buf[2]) << 8 | (uint8_t)buf[3];
			request_info.ip = (((uint8_t)buf[4])<<24 | ((uint8_t)buf[5])<<16 | ((uint8_t)buf[6])<<8 | (uint8_t)buf[7]);
			
			std::cerr << "request_info.vn: " << (int) request_info.vn << std::endl;
			std::cerr << "request_info.cd: " << (int) request_info.cd << std::endl;
			std::cerr << "request_info.port " << (int) request_info.port << std::endl;
			std::cerr << "request_info.ip: " <<  request_info.ip << std::endl;
		}
	}
	void get_reply(Client& client)
	{
		char buf[9];
		int len = 0;
		int len2 = 0;
		len = read(client.sockfd, buf, 8);
		//for(int i=0; i<len; i++)
		//	std::cerr << "buf " << (int) buf[i] << std::endl;

		parse_request(buf, len);
		//std::cerr << "fffget_reply len " << len << " len2 " << len2 << std::endl;
	}
	void send_request(Client& client)
	{
		unsigned char pack[9];
		
		pack[0] = 4;
		pack[1] = 1;
		pack[2] = client.socks_port/256;
		pack[3] = client.socks_port%256;
		pack[4] = client.socks_ip >> 24;
		pack[5] = (client.socks_ip>>16) & 0xFF;
		pack[6] = (client.socks_ip>>8) & 0xFF;
		pack[7] = client.socks_ip & 0xFF;
		pack[8] = 0;
		
		//std::cerr << (int) pack[1] << std::endl;
		int len = write(client.sockfd, pack, 9);
		//std::cerr << "send r " << len << " " << client.socks_ip << " " << client.socks_port << std::endl;
	}
	
	void get_query_string()
	{
		query_strings.clear();
		
		std::string query_string = "h1=140.113.235.169&p1=5555&f1=t1.txt&h2=140.113.235.169&p2=5555&f2=t2.txt&h3=140.113.235.169&p3=5555&f3=t3.txt&h4=140.113.235.169&p4=5555&f4=t4.txt&h5=140.113.235.169&p5=5555&f5=t5.txt"; 
		const char* env = getenv("QUERY_STRING");
		if(env)
			query_string = env;
		
		std::vector<std::string> tmp_vec;
		boost::split(tmp_vec, query_string, boost::is_any_of("=&"), boost::token_compress_off);
		for(int i=1; i<tmp_vec.size(); i+=2)
		{
			query_strings[tmp_vec[i-1]] = tmp_vec[i];
			//result.push_back({tmp_vec[i-1], tmp_vec[i]});
			std::cerr << "Query string : " << tmp_vec[i-1] << " = " << tmp_vec[i] << std::endl;
		}
	}
	void print_header()
	{
		std::string header = R"(
			<html>
	        <head>
	        <meta http-equiv="Content-Type" content="text/html; charset=big5" />
	        <title>Network Programming Homework 3</title>
	        </head>
	        <body bgcolor=#336699>
	        <font face="Courier New" size=2 color=#FFFF99>
	        <table width="800" border="1">
	        <tr>
	    )";
	    std::cout << header ;
	    
	    for(auto &client : clients)
		    std::cout << "<td>" << client.ip << "</td>";
	    
	    std::cout << "</tr><tr>";
	    for(auto &client : clients)
		    std::cout << "<td valign=\"top\" id=\"m" << client.id << "\"></td>";
	    
	    std::cout << "</tr></table>";
	    
		std::cout << std::endl;
		std::cout.flush();
	}
	void print_footer()
	{
		std::string footer = R"(
			</font>
			</body>
			</html>
		)";
		std::cout << footer << std::endl;
		std::cout.flush();
	}
};





int main(int argc, char** argv)
{
	std::cout << "Content-Type: text/html\n\n";
	std::cout.flush();
	
	cgi hw3;
	hw3.init();
	hw3.print_header();
	
	hw3.run();
	hw3.print_footer();
	//std::vector<std::pair<std::string, std::string>> query_strings = get_query_string();
	
	//std::cout << "Test OK" << std::endl;
	//std::cout << "Test OK" << std::endl;
	//std::cout << "Test OK" << std::endl;
	return 0;
}