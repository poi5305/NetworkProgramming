#ifndef __MULTIPLE_PROCESS__
#define __MULTIPLE_PROCESS__

#include "shell.hpp"

class multiple_process
{
	int user_id;
	int socket_fd;
public:
	
	multiple_process()
	{
		// broad cast
		signal(SIGUSR1, [](int k){
			struct_utility::lock();
			std::cout << p_shared->broadcast;
			//if(p_shared->broadcast_user != shell<multiple_process>::g_user_id)
			//	std::cout << "% ";
			std::cout.flush();
			struct_utility::unlock();
		});
		// tell msg
		signal(SIGUSR2, [](int k)
		{
			struct_utility::lock();
			for(int i=0; i<FIFO_LEN; i++)
			{
				auto &fifo = p_shared->chart_fifo[i];
				if(fifo.state == 2 && fifo.from_user == shell<multiple_process>::g_user_id && fifo.to_user != shell<multiple_process>::g_user_id)
				{
					close(fifo.read_fd);
					close(fifo.write_fd);
				}
			}
		
			for(int i=0; i<MSG_NUM; i++)
			{
				auto &msg = p_shared->msgs[i];
				if(msg.state == 1 && msg.to_user == shell<multiple_process>::g_user_id)
				{
					msg.state = 0;
					std::cout << msg.msg;
					//if(msg.from_user != shell<multiple_process>::g_user_id)
					//	std::cout << "% ";
					std::cout.flush();
				}
			}
			struct_utility::unlock();
		});
	}
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
	{}
	void update_fd(int fd)
	{}
	void yell(const std::string &msg)
	{
		struct_utility::lock();
		strncpy(p_shared->broadcast, msg.c_str(), MSG_LEN);
		p_shared->broadcast_user = user_id;
		console::debug("kill yell "+std::to_string(users[user_id].pid));
		struct_utility::unlock();
		//kill(users[user_id].pid, SIGUSR1);
		for(int i=1; i< USER_LEN; i++)
		{
			if(users[i].state == 0)
				continue;
			kill(users[i].pid, SIGUSR1);
		}
		//kill(0, SIGUSR1);
	}
	void tell(int to_user_id, const std::string &msg)
	{
		//int msg_id = get_msg_new_id();
		struct_utility::lock();
		int msg_id = struct_utility::get_new_id(p_shared->msgs, MSG_NUM);
		strncpy(p_shared->msgs[msg_id].msg, msg.c_str(), MSG_LEN);
		p_shared->msgs[msg_id].state = 1;
		p_shared->msgs[msg_id].from_user = user_id;
		p_shared->msgs[msg_id].to_user = to_user_id;
		struct_utility::unlock();
		//kill(users[user_id].pid, SIGUSR2);
		kill(users[to_user_id].pid, SIGUSR2);
	}
	void create_fifo(std::vector<command> &cmds, int pipe_i)
	{
		auto &cmd = cmds[pipe_i];
		if(cmd.user_out != 0)
		{
			//std::cerr << "A" << user_id << " " << cmd.user_out << std::endl;
			std::string fifo_name;
			fifo_name += "/tmp/andy_fifo_" + std::to_string(user_id) + "_" + std::to_string(cmd.user_out);
			
			console::debug(fifo_name);
			
			unlink(fifo_name.c_str());
			if( mknod( fifo_name.c_str(),  S_IFIFO | 0666, 0 ) < 0 )
				console::error("Can't mkfifo");
			int fifo_0, fifo_1;
			
			if( (fifo_0 = open( fifo_name.c_str(), O_RDONLY|O_NONBLOCK )) < 0)
				console::error("Can't open fifo 0");
			if( (fifo_1 = open( fifo_name.c_str(), O_WRONLY )) < 0)
				console::error("Can't open fifo 1");
				
			struct_utility::lock();
			//int chart_fifo_new_id = get_chart_fifo_new_id();
			int chart_fifo_new_id = struct_utility::get_new_id(p_shared->chart_fifo, FIFO_LEN);
			chart_fifo[chart_fifo_new_id].state = 1;
			chart_fifo[chart_fifo_new_id].from_user = user_id;
			chart_fifo[chart_fifo_new_id].to_user = cmd.user_out;
			chart_fifo[chart_fifo_new_id].read_fd = fifo_0;
			chart_fifo[chart_fifo_new_id].write_fd = fifo_1;
			struct_utility::unlock();
		}
		if(cmd.user_in != 0)
		{
			//std::cerr << "A" << cmd.user_in << " " << user_id << std::endl;
			std::string fifo_name;
			fifo_name += "/tmp/andy_fifo_" + std::to_string(cmd.user_in) + "_" + std::to_string(user_id);
			
			console::debug(fifo_name+"in");
			int fifo_0, fifo_1;
			
			if( (fifo_0 = open( fifo_name.c_str(), O_RDONLY )) < 0)
				console::error("Can't open fifo 0");
			
			struct_utility::lock();
			int chart_fifo_id = struct_utility::find(chart_fifo, cmd.user_in, user_id);
			//std::cerr << "outfork read fd " << fifo_0 <<" chart_fifo_id "<< chart_fifo_id << std::endl;
			//std::cerr << "user_in " << fifo_0 << socket_fd << std::endl;
			chart_fifo[chart_fifo_id].read_fd = fifo_0;
			chart_fifo[chart_fifo_id].state = 2;
			struct_utility::unlock();
			kill(users[ cmd.user_in ].pid, SIGUSR2);
			// echo in to close fifo write
			
			
			
		}
	}
	void exit_impl()
	{
		exit(0);
	}
};

#endif