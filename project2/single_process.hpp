#ifndef __SINGLE_PROCESS__
#define __SINGLE_PROCESS__

#include "shell.hpp"

class single_process
{
public:
	int user_id;
	int socket_fd;
public:
	single_process()
	{}
	void set_id(int uid, int sockfd)
	{
		user_id = uid;
		socket_fd = sockfd;
	}
	void print_success()
	{
		std::cout << "% ";
		std::cout.flush();
	}
	void update_fd()
	{
		if(socket_fd < 3)
			return;
		//console::error("Error, update_fd can't smaller than 3");
		std::cerr << "close 0, 1. dup " << socket_fd << std::endl;
		close(0);dup(socket_fd);
		close(1);dup(socket_fd);
		//close(2);dup(socket_fd);
	}
	void update_fd(int fd)
	{
		if(fd < 3)
			return;
		//console::error("Error, update_fd can't smaller than 3");
		std::cerr << "close 0, 1. dup " << fd << std::endl;
		close(0);dup(fd);
		close(1);dup(fd);
		//close(2);dup(socket_fd);
	}
	void yell(const std::string &msg)
	{
		for(int i = 1; i<USER_LEN; ++i)
		{
		  int u_user_id = i;
		  auto &u_user = users[u_user_id];
		  if(u_user.state == 0)
		  	continue;
		  std::cout.flush();
		  update_fd(u_user.socket_fd);
		  std::cout << msg ;
		  if( u_user_id != user_id)
		  	print_success();
		  std::cout.flush();
		}
		update_fd();
	}
	void tell(int to_user_id, const std::string &msg)
	{
		int to_fd = users[to_user_id].socket_fd;
		console::debug(std::string("tell to ")+std::to_string(to_user_id)+" "+msg);
		update_fd(to_fd);
		std::cout << msg ;
		if( to_user_id != user_id)
		  print_success();
		std::cout.flush();
		update_fd(socket_fd);
	}
	void create_fifo(std::vector<command> &cmds, int pipe_i)
	{
		auto &cmd = cmds[pipe_i];
		if(cmd.user_out != 0)
		{
			std::cerr << "A" << user_id << " " << cmd.user_out << std::endl;
			std::string fifo_name;
			fifo_name += "fifo_" + std::to_string(user_id) + "_" + std::to_string(cmd.user_out);
			
			console::debug(fifo_name);
			
			unlink(fifo_name.c_str());
			if( mknod( fifo_name.c_str(),  S_IFIFO | 0666, 0 ) < 0 )
				console::error("Can't mkfifo");
			int fifo_0, fifo_1;
			
			if( (fifo_0 = open( fifo_name.c_str(), O_RDONLY|O_NONBLOCK )) < 0)
				console::error("Can't open fifo 0");
			if( (fifo_1 = open( fifo_name.c_str(), O_WRONLY )) < 0)
				console::error("Can't open fifo 1");
				
			//int chart_fifo_new_id = get_chart_fifo_new_id();
			int chart_fifo_new_id = struct_utility::get_new_id(p_shared->chart_fifo, FIFO_LEN);
			chart_fifo[chart_fifo_new_id].state = 1;
			chart_fifo[chart_fifo_new_id].from_user = user_id;
			chart_fifo[chart_fifo_new_id].to_user = cmd.user_out;
			chart_fifo[chart_fifo_new_id].read_fd = fifo_0;
			chart_fifo[chart_fifo_new_id].write_fd = fifo_1;
			
		}
		if(cmd.user_in != 0)
		{}
	}
	void exit_impl()
	{}
};


#endif