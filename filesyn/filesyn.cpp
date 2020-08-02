// filesyn.cpp: 定义应用程序的入口点。
//

#include "filesyn.h"
#include "boost/filesystem.hpp"
#include "fileSynConfig.h"
using namespace std;

int main()
{
	cout << fileSyn_VERSION_MAJOR << "." << fileSyn_VERSION_MINOR << endl;
	return 0;
}
