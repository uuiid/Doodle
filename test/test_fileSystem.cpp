/*
 * @Author: your name
 * @Date: 2020-09-02 13:21:54
 * @LastEditTime: 2020-12-01 11:40:38
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\test\test_fileSystem.cpp
 */
#include "fileSystem_global.h"

#include "src/DfileSyntem.h"
#include "src/ftpsession.h"

#include "Logger.h"
#include <QFileInfo>
#include <QDateTime>
#include <gtest/gtest.h>

#include <QDir>
#include <boost/filesystem/operations.hpp>
TEST(ftp, upload) {
  doSystem::ftpSessionPtr session = doSystem::DfileSyntem::getFTP().session("192.168.10.213",
                                                                            21,
                                                                            "dubuxiaoyaozhangyubin",
                                                                            "zhangyubin");
  ASSERT_TRUE(session->upload("D:/tmp/BuJu.1002.png", "/cache/test/test.png"));
}
TEST(ftp, down) {
  doSystem::ftpSessionPtr session = doSystem::DfileSyntem::getFTP().session("192.168.10.213",
                                                                            21, "", "");
  ASSERT_TRUE(session->down("D:/tmp/test.exe", "/dist/doodle.exe"));
}

TEST(ftp, getInfo) {
  doSystem::ftpSessionPtr session = doSystem::DfileSyntem::getFTP().session("192.168.10.213",
                                                                            21, "", "");
  doSystem::oFileInfo info = session->fileInfo("/dist/doodle.exe");
  DOODLE_LOG_DEBUG << QDateTime::fromTime_t(info.fileMtime);
  DOODLE_LOG_DEBUG << (info.fileSize) / (1024 * 1024) << "/mb";
  DOODLE_LOG_DEBUG << info.filepath.c_str();
  DOODLE_LOG_DEBUG << info.isFolder;
}
TEST(ftp, getInfo_folder) {
  doSystem::ftpSessionPtr session = doSystem::DfileSyntem::getFTP().session("192.168.10.213",
                                                                            21, "", "");
  doSystem::oFileInfo info = session->fileInfo("/dist/");
  DOODLE_LOG_DEBUG << QDateTime::fromTime_t(info.fileMtime);
  DOODLE_LOG_DEBUG << (info.fileSize) / (1024 * 1024) << "/mb";
  DOODLE_LOG_DEBUG << info.filepath.c_str();
  DOODLE_LOG_DEBUG << info.isFolder;
}

TEST(ftp, getList) {
  doSystem::ftpSessionPtr session = doSystem::DfileSyntem::getFTP().session("192.168.10.213",
                                                                            21, "", "");
  std::vector<doSystem::oFileInfo> info = session->list("/dist/");
  for (unsigned int i = 0; i < info.size(); ++i) {
    DOODLE_LOG_DEBUG << "is folder" << info[i].isFolder;
    DOODLE_LOG_DEBUG << "path :" << info[i].filepath.c_str();
  }
}
TEST(ftp, downFolder) {
  //出错几率不高, 可能有极小一部分部分没有下载
  doSystem::ftpSessionPtr session = doSystem::DfileSyntem::getFTP().session("192.168.10.213",
                                                                            21, "", "");
  DOODLE_LOG_DEBUG << QDir::cleanPath("/dist/");
  ASSERT_TRUE(session->downFolder("D:/tmp/tt", "/dist/"));
}
TEST(ftp, uploadFolder) {
  doSystem::ftpSessionPtr session = doSystem::DfileSyntem::getFTP().session("192.168.10.213",
                                                                            21,
                                                                            "dubuxiaoyaozhangyubin",
                                                                            "zhangyubin");
  ASSERT_TRUE(session->uploadFolder("D:/tmp/render", "/cache/test"));
}
TEST(ftp, createFolder) {
  doSystem::ftpSessionPtr session = doSystem::DfileSyntem::getFTP().session("192.168.10.213",
                                                                            21,
                                                                            "dubuxiaoyaozhangyubin",
                                                                            "zhangyubin");
  ASSERT_TRUE(session->createDir("/tmp/test/dddd"));
}
TEST(ftp, copy_dir_not_backup) {
  doSystem::DfileSyntem::copy(R"(D:/tmp/tt)", "D:/tmp/tt2", false);
}
TEST(ftp, copy_dir_backup) {
  doSystem::DfileSyntem::copy(R"(D:/tmp/tt)", "D:/tmp/tt2", true);
}
TEST(ftp, copy_file_backup) {
  doSystem::DfileSyntem::copy(R"(D:/tmp/tt/doodle.exe)", "D:/tmp/tt3/doodle.exe", true);
}
TEST(ftp, copy_dir_ip) {
  doSystem::DfileSyntem::copy(R"(\\192.168.10.253\Prism_projects\cache)", "D:/tmp/tt2", false);
}
TEST(ftp, rename) {
  try {
    boost::filesystem::rename(R"(\\192.168.10.213\sda\test.txt)", R"(\\192.168.10.213\sda\test2.txt)");

  } catch (boost::filesystem::filesystem_error &err) {
    std::string test = err.what();
    std::cout << err.what() << std::endl;
    auto st = boost::filesystem::status(R"(\\192.168.10.213\sda\test.txt)");
    std::cout << st.permissions();
  }
}