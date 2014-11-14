#ifndef __PARSER__
#define __PARSER__

#include <iostream>
#include <string>
#include <vector>
#include <map>


struct command
{
	int pipe_num;
	int command_idx;
	bool is_end;
	std::vector<std::string> argv;
	std::string file1;
	std::string file2;
	int user_out;
	int user_in;
};

class parser
{
public:
	std::string line;
	std::vector<command> parse_line()
	{
		line = "";
		std::vector<command> pipes;
		std::vector<std::string> argvs;
		std::string argv, file1, file2;
		int file_num = 0, pipe_num = 1;
		int user_out = 0, user_in = 0;
		
		while(true)
		{
			char c = std::cin.get();
			if(!std::cin.good() || std::cin.eof())
				exit(0);	
			line.push_back(c);
			
			if(c == '"')
				std::getline(std::cin, argv, '"');
			else if(c == '\'')
				std::getline(std::cin, argv, '\'');
			
			if(c == ' ' || c == '|' || c == '\n')
			{
				if(argv.size() != 0)
				{
					argv.erase(std::remove(argv.begin(), argv.end(), '\r'), argv.end());
					argvs.push_back(argv);
					argv = "";
				}
			}
			if(c == ' ' || c == '\r') 
			{}
			else if(c == '/')
			{
				syntax_error(c);
				argv = ""; file1 = ""; file2 = ""; user_out = 0; user_in = 0;
				argvs.resize(0); pipes.resize(0);
				file_num=0; pipe_num=1;
			}
			else if(c == '|')
			{
				while(true)
				{
					char nc = std::cin.peek();
					if(nc >= 48 && nc <= 57)// is 0~9
						argv.push_back(std::cin.get());
					else
						break;
				}
				pipe_num = std::atoi(argv.c_str());
				if(pipe_num == 0)
					pipe_num = 1;
				pipes.push_back({pipe_num, 0, false, argvs, file1, file2, user_out, user_in});
				argv = ""; file1 = ""; file2 = ""; user_out = 0; user_in = 0;
				argvs.resize(0);				
				pipe_num = 1;
			}
			else if(c == '>')
			{
				int file_num = 1;
				// ls >f, ls > f
				if(argv.size() != 0)
				{ // ls 2> f, ls 2>f
					int num = std::atoi(argv.c_str());
					if(num == 0) // ls>f
						argvs.push_back(argv);
					else
						file_num = num;
				}
				argv = "";
				int tag = parse_next_word(argv);
				if( tag == 0)
				{
					syntax_error(std::cin.peek());
					pipes.resize(0); argvs.resize(0);
				}
				int to_user = atoi(argv.c_str());
				if(tag == 1 || to_user == 0) // file
				{
					if(file_num == 1)
						file1 = argv;
					else
						file2 = argv;
				}
				else
				{
					user_out = to_user;
				}
				argv = "";
			}
			else if(c == '<')
			{
				int file_num = 1;
				// ls >f, ls > f
				if(argv.size() != 0)
				{ // ls 2> f, ls 2>f
					int num = std::atoi(argv.c_str());
					if(num == 0) // ls>f
						argvs.push_back(argv);
					else
						file_num = num;
				}
				argv = "";
				int tag = parse_next_word(argv);
				if( tag == 0)
				{
					syntax_error(std::cin.peek());
					pipes.resize(0); argvs.resize(0);
				}
				int to_user = atoi(argv.c_str());
				if(tag == 1 || to_user == 0) // file
				{}
				else
				{
					user_in = to_user;
				}
				argv = "";
			}
			else if(c == '\n')
			{
				if(argvs.size() > 0)
					pipes.push_back({pipe_num, 0, true, argvs, file1, file2, user_out, user_in});
				break;
			}
			else
				argv.push_back(c);
		}
		return pipes;
	}
private:
	int parse_next_word(std::string &argv)
	{
		int tag = 2; // 0=null, 1=file, 2=to_user
		while(std::cin.peek() == ' ')
		{
			tag = 1;
			std::cin.get();
		}
		while(true)
		{
			char nc = std::cin.peek();
			if(nc == '|' || nc == ' ' || nc == '\n' || nc == '\r')
				break;
			else
				argv.push_back(std::cin.get());
		}
		if(argv.size() == 0)
			tag = 0;
		return tag;
	}
	void syntax_error(char c)
	{
		while(true)
		{
			char k = std::cin.get();
			if(k == '\n')
				break;
		}
		std::cerr << "syntax error near unexpected token '"<< c << "'" << std::endl;
		std::cout << "% ";
		std::cout.flush();
	}
};

#endif