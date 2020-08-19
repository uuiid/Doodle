#include "fileInfo.h"

const char* fileInfo::install = "INSERT INTO @table(file_size,modify_time,path) \
VALUES (@file_size,@modify_time,@path)";

const char* fileInfo::createTable = "CREATE TABLE IF NOT EXISTS @firstTable (\
    id integer primary key AUTOINCREMENT,\
    file_size integer,\
    modify_time text,\
    path text\
    )";

fileInfo::fileInfo( )
{
    fileSize = NULL;
    modifyTime = boost::posix_time::from_time_t(NULL);
    synTimer = boost::posix_time::from_time_t(NULL);
    _absPath_ = boost::filesystem::path();
    relativePath = boost::filesystem::path( );

    tableName = "";
}

fileInfo::fileInfo(boost::filesystem::path absPath, boost::filesystem::path root)
{
    fileSize = boost::filesystem::file_size(absPath);
    time_t time = boost::filesystem::last_write_time(absPath);
    modifyTime = boost::posix_time::from_time_t(time);
    synTimer = boost::posix_time::from_time_t(NULL);

    relativePath = boost::filesystem::relative(root, absPath);

    _absPath_ = absPath;
    tableName = "";
}

fileInfo::~fileInfo( )
{
}

void fileInfo::setTableName(std::string& table)
{
    tableName = table;
}

sqlite3_stmt* fileInfo::getCreateTable(sqlite3* sqlDB,sqlite3_stmt* sqlStmt,std::string table)
{

    sqlite3_prepare(sqlDB,createTable, -1, &sqlStmt, NULL);
    sqlite3_bind_text(sqlStmt,1,table.c_str(),-1,NULL);
    return sqlStmt;
}

sqlite3_stmt* fileInfo::bind_prepare(sqlite3_stmt* sqlStmt)
{
    sqlite3_bind_int64(sqlStmt,1,fileSize);

    sqlite3_bind_text(sqlStmt, 2, boost::posix_time::to_iso_extended_string(modifyTime).c_str(),-1,NULL);
    sqlite3_bind_text(sqlStmt, 3, relativePath.string().c_str(), -1, NULL);
    return sqlStmt;
}

