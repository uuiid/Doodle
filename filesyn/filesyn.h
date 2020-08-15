// filesyn.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#pragma once


#include "setting.h"
#include <boost/filesystem.hpp>
#include <iostream>

/// <summary>
/// 文件夹对为兼容不同的传输协议做的
/// </summary>
class folderCompare
{
public:
	folderCompare( );
	folderCompare(boost::filesystem::path path1, boost::filesystem::path path2);
	~folderCompare( );

	void setFolderCompare(boost::filesystem::path path1, boost::filesystem::path path2);
	boost::filesystem::path getCompareFirst( );
	boost::filesystem::path getCompareSecond( );
	std::pair<boost::filesystem::path, boost::filesystem::path> getFolderCompare( );

private:
	std::pair<boost::filesystem::path, boost::filesystem::path> compare;
};


/// <summary>
/// 主要同步类
/// </summary>
class fileSyn
{
public:
	fileSyn( );
	~fileSyn( );
	void scan( );

	void addFolderCompare(folderCompare folderCom);
	void clearFodlerCompare( );
private:
	void scanfile(boost::filesystem::path root);
	std::vector<folderCompare> folder;
};
// TODO: 在此处引用程序需要的其他标头。
