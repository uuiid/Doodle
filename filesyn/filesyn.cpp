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

fileSyn::fileSyn( )
{
}

fileSyn::~fileSyn( )
{
}

void fileSyn::scan( )
{
}

void fileSyn::scanfile(boost::filesystem::path root)
{
	if (!boost::filesystem::exists(root))
	{
		return ;
	}
	//globalSetting* test = &globalSetting::GetSetting( );
	//globalSetting::GetSetting( ).setRoot(root);
	if (boost::filesystem::is_directory(root))
	{
		boost::filesystem::recursive_directory_iterator end_iter;
		
		for (boost::filesystem::recursive_directory_iterator iter(root); iter != end_iter; iter++)
		{
			try
			{
				if (boost::filesystem::is_regular_file(iter->path()))
				{
					std::cout << iter->path( ).string( ) << std::endl;
				}
			}
			catch (const std::exception& ex)
			{
				std::cout << "错误" << ex.what() << std::endl;
				continue;
			}
		}

	}
	
}

void fileSyn::addFolderCompare(folderCompare &folderCom)
{
	folder.push_back(&folderCom);
}

void fileSyn::clearFodlerCompare( )
{
	folder.clear( );
}


int main()
{
	std::cout << fileSyn_VERSION_MAJOR << "." << fileSyn_VERSION_MINOR << std::endl;
	fileSyn f;
	boost::filesystem::path p("D:\\USD\\plugin\\usd");
	
	//f.scanfile(p);
	
	return 0;
}
