#include "folderCompare.h"
#include <boost/filesystem.hpp>

folderCompare::folderCompare( )
{
}

folderCompare::folderCompare(boost::filesystem::path path1, boost::filesystem::path path2)
{
	compare = std::make_pair(path1, path2);
}

folderCompare::~folderCompare( )
{
}

void folderCompare::setFolderCompare(boost::filesystem::path &path1, boost::filesystem::path &path2)
{
	compare = std::make_pair(path1, path2);
}

boost::filesystem::path folderCompare::getCompareFirst( )
{
	return compare.first;
}

boost::filesystem::path folderCompare::getCompareSecond( )
{
	return compare.second;
}

std::pair<boost::filesystem::path, boost::filesystem::path> folderCompare::getFolderCompare( )
{
	return compare;
}


folderCompareSys::folderCompareSys( )
{
}

folderCompareSys::~folderCompareSys( )
{
}

void folderCompareSys::scan( )
{
	scanPath(compare.first);
	scanPath(compare.second);
}

void folderCompareSys::scanPath(boost::filesystem::path& path)
{
	if (!boost::filesystem::exists(path))
	{
		return;
	}
	//globalSetting* test = &globalSetting::GetSetting( );
	//globalSetting::GetSetting( ).setRoot(root);
	if (boost::filesystem::is_directory(path)) //确认是目录
	{
		boost::filesystem::recursive_directory_iterator end_iter;
		//创建迭代器开始迭代
		for (boost::filesystem::recursive_directory_iterator iter(path); iter != end_iter; iter++)
		{
			try
			{//错误处理, 并创建文件信息类
				if (boost::filesystem::is_regular_file(iter->path( )))
				{
					boost::shared_ptr<fileInfo> info(new fileInfo(iter->path( )));
					fileInfoPtr.push_back(info);
				}
			}
			catch (const std::exception& ex)
			{
				std::cout << "错误: " << ex.what( ) << std::endl;
				continue;
			}
		}

	}
}

void folderCompareFtp::scan( )
{

}

folderCompareFtp::folderCompareFtp( )
{
}

folderCompareFtp::~folderCompareFtp( )
{
}

