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


struct HTTPD_PARA
{
	std::string method;
	std::string path;
	std::string file;
	std::string sub_file;
	std::string query;
	std::string proto;
};

class HTTPD
{
	int sockfd;
	std::vector<std::string> client_msgs;
	std::string path;
	HTTPD_PARA httpd_para;
public:
	HTTPD(int fd)
		:sockfd(fd), path("/u/other/2015_1/c0057204/public_html"), httpd_para()
	{
		dup2(sockfd, 0);
		dup2(sockfd, 1);
		
		parse_client_msg();
		parse_get();
		std::cout << "HTTP/1.1 200 OK\n";
		run();
	}
	void run()
	{
		std::string filename = test_filename(httpd_para.file);
		if(filename == "")
		{
			std::cout << "Content-type: text/html; charset=big5\n\n";
			std::cout << "<h1>404 NOT FOUND</h1>\n";
			std::cout << filename << "<br />";
			return;
		}
		int state = 0;
		if(httpd_para.sub_file == "html")
		{
			state = run_html();
		}
		else // cgi
		{
			state = run_cgi();
		}
		if(state == -1)
		{
			std::cout << "Content-type: text/html; charset=big5\n\n";
			std::cout << "<h1>403 Access deny!</h1>\n";
		}
	}
	int run_html()
	{
		std::fstream fp(httpd_para.file);
		std::string line;
		while (std::getline(fp, line))
		{
			std::cout << line << "\n";
		}
		fp.close();
		return 0;
	}
	int run_cgi()
	{
		const char *argv[2];
		argv[0] = httpd_para.file.c_str();
		argv[1] = NULL;
		
		set_env();
		
		std::string filename = test_filename(httpd_para.file);
		std::cerr << filename << " " << argv[0] << std::endl;
		
		if (execvp(filename.c_str(), (char **)argv) < 0)
		{
			perror("exec");
			return -1;
		}
		return 0;
	}
	void set_env()
	{
		setenv("QUERY_STRING", httpd_para.query.c_str(), 1);
		setenv("CONTENT_LENGTH", "9999", 1);
		setenv("REQUEST_METHOD", httpd_para.method.c_str(), 1);
		setenv("SCRIPT_NAME", httpd_para.file.c_str(), 1);
		setenv("REMOTE_HOST", "somewhere.nctu.edu.tw", 1);
		setenv("REMOTE_ADDR", "140.113.1.1", 1);
		setenv("AUTH_TYPE", "auth_type", 1);
		setenv("REMOTE_USER", "remote_user", 1);
		setenv("REMOTE_IDENT", "remote_ident", 1);
	}
	std::string test_filename(std::string &f)
	{
		std::string filename = path + "/" + f;
		if( access( filename.c_str(), F_OK ) != -1)
			return filename;	
		return "";
	}
	void parse_client_msg()
	{
		std::string line;
		while(std::getline(std::cin, line))
		{
			if(line.size() <= 1)
				break;
			client_msgs.push_back(line);
			//std::cout << line << std::endl;
		}
	}
	void parse_get()
	{
		//GET /~c0057204/hw3.cgi?h1=140.113.235.169&p1=5555&f1=t1.txt&h2=140.113.235.169&p2=5555&f2=t2.txt&h3=140.113.235.169&p3=5555&f3=t3.txt&h4=140.113.235.169&p4=5555&f4=t4.txt&h5=140.113.235.169&p5=5555&f5=t5.txt
		std::string str, file, sub_file;
		auto &line = client_msgs[0];
		for(int i=0; i<line.size();i++)
		{
			char c = line[i];
			if(c == '?')
			{
				httpd_para.path = str;
				httpd_para.file = file;
				httpd_para.sub_file = sub_file;
				str = "";
			}
			else if(c == ' ')
			{
				if(httpd_para.method.size() == 0)
					httpd_para.method = str;
				else if(httpd_para.path.size() == 0)
				{
					httpd_para.path = str;
					httpd_para.file = file;
					httpd_para.sub_file = sub_file;
				}
				else
					httpd_para.query = str;
				str = "";
			}
			else if(c == '.')
			{
				str += c;
				file += c;
				sub_file = "";
			}
			else if(c == '/')
			{
				str += c;
				file = "";
			}
			else
			{
				str += c;
				file += c;
				sub_file += c;
			}
			httpd_para.proto = str;
		}
		std::cerr << "method: " << httpd_para.method << "\n";
		std::cerr << "path: " << httpd_para.path << "\n";
		std::cerr << "file: " << httpd_para.file << "\n";
		std::cerr << "sub_file: " << httpd_para.sub_file << "\n";
		std::cerr << "query: " << httpd_para.query << "\n";
		std::cerr << "proto: " << httpd_para.proto << "\n";
		
	}
	
};


int main(int argc, char** argv)
{
	signal(SIGUSR1, [](int k){});
	signal(SIGUSR2, [](int k){});
	signal(SIGCHLD, SIG_IGN); // signal(SIGCHLD, childhandler);
	std::function<void(server_info)> run_httpd = [](server_info s_info)
	{
		HTTPD httpd(s_info.sockfd);
	};
	
	
	server::forked_server("5566", run_httpd);
	
	return 0;
}