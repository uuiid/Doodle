// filesyn.cpp: 定义应用程序的入口点。
//

#include "filesyn.h"
#include "boost/filesystem.hpp"
#include "fileSynConfig.h"


int main()
{
	std::cout << fileSyn_VERSION_MAJOR << "." << fileSyn_VERSION_MINOR << std::endl;
	return 0;
}
