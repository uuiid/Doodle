// filesyn.cpp: 定义应用程序的入口点。
//

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include "filesyn.h"
#include "fileInfo.h"
#include "setting.h"
#include "fileSynConfig.h"

/// <summary>
/// 主要同步类
/// ==================================================================
/// </summary>

typedef boost::shared_ptr<folderCompare> test;

fileSyn::fileSyn( )
{
}

fileSyn::~fileSyn( )
{
}

void fileSyn::scan( )
{
	std::list<boost::shared_ptr<folderCompare>>::iterator it;
	for(it = folder.begin();it != folder.end(); ++it)
	{
		(**it).scan( );
	}
}


void fileSyn::addFolderCompare(folderCompare &folderCom)
{
	boost::shared_ptr<folderCompare> ptr(&folderCom);
	folder.push_back(ptr);
}

void fileSyn::clearFodlerCompare( )
{
	folder.clear( );
}

inline void EnableMemLeakCheck( )
{
	//该语句在程序退出时自动调用 _CrtDumpMemoryLeaks(),用于多个退出出口的情况.
	//如果只有一个退出位置，可以在程序退出之前调用 _CrtDumpMemoryLeaks()
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
}

int main()
{
	EnableMemLeakCheck( );
	std::cout << fileSyn_VERSION_MAJOR << "." << fileSyn_VERSION_MINOR << std::endl;
	fileSyn f;
	boost::filesystem::path p("F:\\USD\\plugin\\usd");
	boost::filesystem::path p2("D:\\job");
	folderCompareSys fcom;
	fcom.setFolderCompare(p, p2);
	f.addFolderCompare(fcom);
	f.scan( );
	//_CrtDumpMemoryLeaks( );
	return 0;
}
