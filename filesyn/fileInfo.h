#pragma once

#include <boost/filesystem.hpp>
#include <boost/progress.hpp>

class fileInfo
{
public:
	fileInfo( );
	~fileInfo( );

	boost::filesystem::path absPath;
	boost::filesystem::path relativePath;
	boost::progress_timer createTime;
	boost::progress_timer modifyTime;
	boost::progress_timer synTimer;


private:

};

fileInfo::fileInfo( )
{
}

fileInfo::~fileInfo( )
{
}