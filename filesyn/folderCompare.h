#pragma once


#include "setting.h"
#include "fileInfo.h"
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
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

	virtual void scan( ) = 0;

	void setFolderCompare(boost::filesystem::path &path1, boost::filesystem::path &path2);
	boost::filesystem::path getCompareFirst( );
	boost::filesystem::path getCompareSecond( );
	std::pair<boost::filesystem::path, boost::filesystem::path> getFolderCompare( );

protected:
	void initDB( );
	std::pair<boost::filesystem::path, boost::filesystem::path> compare;
	std::list<sPtrFileInfo> fileInfoPtrFirst;
	std::list<sPtrFileInfo> fileInfoPtrSecond;
};

typedef boost::shared_ptr<folderCompare> sPtrFolderCompare;

class folderCompareSys :public folderCompare
{
public:
	folderCompareSys( );
	~folderCompareSys( );
	void scan( ) override;
private:
	void scanPath(const boost::filesystem::path & path, std::list<boost::shared_ptr<fileInfo>> &addFileInfo);
};

typedef boost::shared_ptr<folderCompareSys> sPtrFolderCompareSys;

class folderCompareFtp :public folderCompare
{
public:
	folderCompareFtp( );
	~folderCompareFtp( );

	void scan( ) override;
private:

};

typedef boost::shared_ptr<folderCompareFtp> sPtrFolderCompareFtp;