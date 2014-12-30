#ifndef __CONSOLE__
#define __CONSOLE__

#include <string.h>

#include<iostream>
class console
{
public:
	static bool tag;
	
	static void debug(const std::string &msg)
	{
		if(tag)
			std::cerr << "===DEBUG===: " << msg << std::endl;
	}
	static void error(std::string &&msg)
	{
		printf("Error %s\n", strerror(errno));
		std::cerr << msg << std::endl;
		exit(1);
	}
	static void log(std::string &&msg)
	{
		std::cout << msg << std::endl;
	}
};

bool console::tag = false;

#endif