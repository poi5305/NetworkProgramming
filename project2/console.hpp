#ifndef __CONSOLE__
#define __CONSOLE__

#include<iostream>
class console
{
public:
	static void debug(const std::string &msg)
	{
		std::cerr << "===DEBUG===: " << msg << std::endl;
	}
	static void error(std::string &&msg)
	{
		std::cerr << msg << std::endl;
		exit(1);
	}
	static void log(std::string &&msg)
	{
		std::cout << msg << std::endl;
	}
};
#endif