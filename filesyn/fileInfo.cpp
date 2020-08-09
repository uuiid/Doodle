#include "fileInfo.h"

fileInfo::fileInfo( )
{
    fileSize = NULL;
    modifyTime = boost::posix_time::from_time_t(NULL);
    synTimer = boost::posix_time::from_time_t(NULL);
    _absPath_ = boost::filesystem::path();
    relativePath = boost::filesystem::path( );
}

fileInfo::fileInfo(boost::filesystem::path absPath)
{
    fileSize = boost::filesystem::file_size(absPath);
    time_t time = boost::filesystem::last_write_time(absPath);
    modifyTime = boost::posix_time::from_time_t(time);
    synTimer = boost::posix_time::from_time_t(NULL);

    _absPath_ = absPath;

    
}

fileInfo::~fileInfo( )
{
}