#pragma once

#include <boost/filesystem.hpp>
#include <boost/date_time.hpp>
//#include <boost/date_time/posix_time/posix_time.hpp>
#include <cstdint>
class fileInfo
{
public:
	fileInfo( );
	fileInfo(boost::filesystem::path absPath );
	~fileInfo( );




private:
	boost::filesystem::path _absPath_;
	boost::filesystem::path relativePath;
	boost::posix_time::ptime modifyTime;
	boost::posix_time::ptime synTimer;
	uintmax_t fileSize;

};

