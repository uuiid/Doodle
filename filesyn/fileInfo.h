#pragma once

#include <boost/filesystem.hpp>
#include <boost/date_time.hpp>
//#include <boost/date_time/posix_time/posix_time.hpp>
#include <cstdint>
#include <sqlite3.h>

class fileInfo
{
public:
	fileInfo( );
	fileInfo(boost::filesystem::path absPath, boost::filesystem::path root);
	~fileInfo( );

	void setTableName(std::string& table);
	// 静态创建文件信息表
	static sqlite3_stmt* getCreateTable(sqlite3* sqlDB, sqlite3_stmt* sqlStmt, std::string table);
	// 编译好后提交的sql语言
	sqlite3_stmt* bind_prepare(sqlite3_stmt* sqlStmt);


private:

	static const char* install;

	static const  char* createTable;

	boost::filesystem::path _absPath_;
	boost::filesystem::path relativePath;
	boost::posix_time::ptime modifyTime;
	boost::posix_time::ptime synTimer;
	uintmax_t fileSize;

	std::string tableName;

};

typedef boost::shared_ptr<fileInfo> sPtrFileInfo;