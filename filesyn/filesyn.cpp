// filesyn.cpp: 定义应用程序的入口点。
//

#include <boost/filesystem.hpp>
#include "filesyn.h"
#include "setting.h"
#include "fileSynConfig.h"

fileSyn::fileSyn( )
{
}

fileSyn::~fileSyn( )
{
}

void fileSyn::scanfile(boost::filesystem::path root)
{
	setting* test = &setting::GetSetting( );
	setting::GetSetting( ).setRoot(root);
	if (boost::filesystem::is_directory(root))
	{
		//synSet.setRoot(root);

	}
	
}


int main()
{
	std::cout << fileSyn_VERSION_MAJOR << "." << fileSyn_VERSION_MINOR << std::endl;
	return 0;
}
