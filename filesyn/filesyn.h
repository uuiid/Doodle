// filesyn.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#pragma once


#include "setting.h"
#include "fileInfo.h"
#include "folderCompare.h"
#include <boost/filesystem.hpp>
#include <iostream>


/// <summary>
/// 主要同步类
/// </summary>
class fileSyn
{
public:
	fileSyn( );
	~fileSyn( );
	void scan( );

	

	void addFolderCompare(folderCompare &folderCom);
	void clearFodlerCompare( );
protected:
	void scanfile(boost::filesystem::path root);
	std::vector<folderCompare*> folder;
};
// TODO: 在此处引用程序需要的其他标头。
