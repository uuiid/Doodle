#include "ftp_global.h"

#include "src/ftphandle.h"
#include "src/ftpsession.h"

#include "Logger.h"
#include <QFileInfo>
#include <QDateTime>
#include <gtest/gtest.h>

#include <QDir>
TEST(ftp, upload) {
  doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session("192.168.10.213",
                                                                    21,
                                                                    "dubuxiaoyaozhangyubin",
                                                                    "zhangyubin");
  ASSERT_TRUE(session->upload("D:/tmp/BuJu.1002.png", "/cache/test/test.png"));

}
TEST(ftp, down)
{
  doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session("192.168.10.213",
                                                                    21, "", "");
  ASSERT_TRUE(session->down("D:/tmp/test.exe", "/dist/doodle.exe"));

}

TEST(ftp,getInfo){
  doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session("192.168.10.213",
                                                                    21, "", "");
  doFtp::oFileInfo info = session->fileInfo("/dist/doodle.exe");
  DOODLE_LOG_DEBUG << QDateTime::fromTime_t(info.fileMtime);
  DOODLE_LOG_DEBUG << (info.fileSize) / (1024 * 1024) << "/mb";
  DOODLE_LOG_DEBUG << info.filepath.c_str();
  DOODLE_LOG_DEBUG << info.isFolder;
}
TEST(ftp,getInfo_folder){
  doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session("192.168.10.213",
                                                                    21, "", "");
  doFtp::oFileInfo info = session->fileInfo("/dist/");
  DOODLE_LOG_DEBUG << QDateTime::fromTime_t(info.fileMtime);
  DOODLE_LOG_DEBUG << (info.fileSize) / (1024 * 1024) << "/mb";
  DOODLE_LOG_DEBUG << info.filepath.c_str();
  DOODLE_LOG_DEBUG << info.isFolder;
}

TEST(ftp,getList){
  doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session("192.168.10.213",
                                                                    21, "", "");
  std::vector<doFtp::oFileInfo> info = session->list("/dist/");
  for (unsigned int i = 0; i < info.size(); ++i) {
    DOODLE_LOG_DEBUG << "is folder" << info[i].isFolder;
    DOODLE_LOG_DEBUG << "path :" << info[i].filepath.c_str();
  }
}
TEST(ftp,downFolder){
  //出错几率不高, 可能有极小一部分部分没有下载
  doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session("192.168.10.213",
                                                                    21, "", "");
  DOODLE_LOG_DEBUG << QDir::cleanPath("/dist/");
  ASSERT_TRUE(session->downFolder("D:/tmp/tt","/dist/"));
}
TEST(ftp,uploadFolder){
  doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session("192.168.10.213",
                                                                    21,
                                                                    "dubuxiaoyaozhangyubin",
                                                                    "zhangyubin");
  ASSERT_TRUE(session->uploadFolder("D:/tmp/render", "/cache/test"));
}
TEST(ftp,createFolder){
  doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session("192.168.10.213",
                                                                    21,
                                                                    "dubuxiaoyaozhangyubin",
                                                                    "zhangyubin");
  ASSERT_TRUE(session->createDir("/tmp/test/dddd"));
}