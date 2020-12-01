#include "ftpsession.h"
#include <curl/curl.h>

#include "Logger.h"
#include <fstream>
#include <QUrl>
#include <QFile>
#include <QDir>

#include <QDirIterator>
#include <memory>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <ctime>
#include <QtCore/QDateTime>

#include <magic_enum.hpp>
#include <boost/numeric/conversion/cast.hpp>
DSYSTEM_S
ftpSession::ftpSession() {
  ptrUrl = std::make_shared<QUrl>();
  outfile = std::make_shared<QFile>();
  inputfile = std::make_shared<QFile>();

  curlSession = curl_easy_init();
}

ftpSession::~ftpSession() { curl_easy_cleanup(curlSession); }

void ftpSession::setInfo(const dstring &host, int prot, const dstring &name,
                         const dstring &password) {
  ptrUrl->setScheme("ftp");
  ptrUrl->setHost(QString::fromStdString(host));
  ptrUrl->setPort(prot);
  if (!name.empty()) {
    ptrUrl->setUserName(QString::fromStdString(name));
  } else {
    ptrUrl->setUserName("anonymous");
  }
  if (!password.empty()) {
    ptrUrl->setPassword(QString::fromStdString(password));
  } else {
    ptrUrl->setPassword("");
  }
}

bool ftpSession::createDir(const std::vector<dstring> &path, bool allPath) {
  if (path.empty()) return false;
  if (!curlSession) return false;

  curl_easy_reset(curlSession);

  std::string comm("MKD ");

  ptrUrl->setPath("/");

  //  comm.append("MKD ").append(path);

  curl_easy_setopt(curlSession, CURLOPT_URL,
                   ptrUrl->toString().toStdString().c_str());
  curl_easy_setopt(curlSession, CURLOPT_NOBODY, 1L);
  curl_easy_setopt(curlSession, CURLOPT_HEADER, 1L);
  curl_easy_setopt(curlSession, CURLOPT_FTP_CREATE_MISSING_DIRS,
                   CURLFTP_CREATE_DIR);

  for (const auto &path_ : path) {
    boost::filesystem::path kpath(path_);

    boost::filesystem::path p;
    if (allPath) {
      for (const auto &item : kpath) {
        p = p / item;
        if (item == *kpath.begin()) {
          continue;
        }
        struct curl_slist *headerList = nullptr;
        headerList =
            curl_slist_append(headerList, (comm + p.generic_string()).c_str());

        curl_easy_setopt(curlSession, CURLOPT_POSTQUOTE, headerList);

        CURLcode err = static_cast<CURLcode>(perform());
        if (err != CURLE_OK && err != CURLE_QUOTE_ERROR) {
          DOODLE_LOG_WARN << err << curl_easy_strerror(err)
                          << ptrUrl->toString().toStdString().c_str();
          break;
        }
        curl_slist_free_all(headerList);
      }
    } else {
      struct curl_slist *headerList = nullptr;
      headerList = curl_slist_append(headerList,
                                     (comm + kpath.generic_string()).c_str());
      curl_easy_setopt(curlSession, CURLOPT_POSTQUOTE, headerList);
      CURLcode err = static_cast<CURLcode>(perform());
      if (err != CURLE_OK && err != CURLE_QUOTE_ERROR)
        DOODLE_LOG_WARN << err << curl_easy_strerror(err)
                        << ptrUrl->toString().toStdString().c_str();
    }
  }
  return true;
}
bool ftpSession::down(const dstring &localFile, const dstring &remoteFile) {
  //打开文件
  outfile->setFileName(QString::fromStdString(localFile));
  if (!outfile->open(QIODevice::WriteOnly)) {
    throw std::runtime_error("not open file");
  }
  //设置url路径
  ptrUrl->setPath(QString::fromStdString(remoteFile));
  curl_easy_reset(curlSession);

  //创建下载设置
  curl_easy_setopt(curlSession, CURLOPT_URL,
                   ptrUrl->toString().toStdString().c_str());
  curl_easy_setopt(curlSession, CURLOPT_WRITEFUNCTION,
                   &ftpSession::writeFileCallbask);
  curl_easy_setopt(curlSession, CURLOPT_WRITEDATA, this->outfile.get());

  //检查下载是否成功
  CURLcode err = static_cast<CURLcode>(perform());
  outfile->close();
  if (err != CURLE_OK) {
    DOODLE_LOG_WARN << err << curl_easy_strerror(err) << outfile->fileName();
    outfile->remove();
    return false;
  }
  return true;
}
bool ftpSession::upload(const dstring &localFile, const dstring &remoteFile) {
  //打开本地文件准备上传
  inputfile->setFileName(QString::fromStdString(localFile));
  if (!inputfile->open(QIODevice::ReadOnly)) {
    throw std::runtime_error("not open file(Read Only)");
  }
  ptrUrl->setPath(QString::fromStdString(remoteFile));
  curl_easy_reset(curlSession);

  //创建上传设置
  curl_easy_setopt(curlSession, CURLOPT_URL,
                   ptrUrl->toString().toStdString().c_str());
  curl_easy_setopt(curlSession, CURLOPT_UPLOAD, 1L);
  curl_easy_setopt(curlSession, CURLOPT_FTP_CREATE_MISSING_DIRS,
                   CURLFTP_CREATE_DIR_RETRY);
  curl_easy_setopt(curlSession, CURLOPT_INFILESIZE, inputfile->size());
  curl_easy_setopt(curlSession, CURLOPT_READFUNCTION,
                   &ftpSession::readFileCallbask);
  curl_easy_setopt(curlSession, CURLOPT_READDATA, inputfile.get());

  CURLcode err = static_cast<CURLcode>(perform());
  inputfile->close();
  if (err != CURLE_OK) {
    return false;
  }
  return true;
}

oFileInfo ftpSession::fileInfo(const dstring &remoteFile) {
  if (remoteFile.empty()) throw std::runtime_error("remote file is NULL");
  ptrUrl->setPath(QDir::cleanPath(QString::fromStdString(remoteFile)));
  curl_easy_reset(curlSession);

  oFileInfo info;

  curl_easy_setopt(curlSession, CURLOPT_URL,
                   ptrUrl->toString().toStdString().c_str());
  curl_easy_setopt(curlSession, CURLOPT_NOBODY, 1L);    //不获取文件本身
  curl_easy_setopt(curlSession, CURLOPT_FILETIME, 1L);  //获得文件时间
  curl_easy_setopt(curlSession, CURLOPT_HEADERFUNCTION,
                   &ftpSession::notCallbask);
  curl_easy_setopt(curlSession, CURLOPT_HEADER, 0L);

  CURLcode err = static_cast<CURLcode>(perform());

  if (err == CURLE_OK) {
    long ftime = -1;
    err = curl_easy_getinfo(curlSession, CURLINFO_FILETIME, &ftime);
    if (err == CURLE_OK && ftime > 0)
      info.fileMtime = static_cast<time_t>(ftime);
    else
      info.fileMtime = -1;

    err = curl_easy_getinfo(curlSession, CURLINFO_CONTENT_LENGTH_DOWNLOAD,
                            &info.fileSize);
    if (err != CURLE_OK || info.fileSize < 0) {
      info.fileSize = 0;
      info.isFolder = true;
    } else {
      info.isFolder = false;
    }

  } else {
    info.fileMtime = -1;
    info.fileSize = 0;
    info.isFolder = true;
  }
  info.filepath =
      QDir::cleanPath(QString::fromStdString(remoteFile)).toStdString();

  return info;
}

/*
获得服务器上的文件列表
*/
std::vector<oFileInfo> ftpSession::list(const dstring &remoteFolder) {
  const QStringList remote =
      QString::fromStdString(remoteFolder).split("/", QString::SkipEmptyParts);
  ptrUrl->setPath(QString::fromStdString(remoteFolder));

  curl_easy_reset(curlSession);  //重置保护
  QString folderList;

  curl_easy_setopt(curlSession, CURLOPT_URL,
                   ptrUrl->toString().toStdString().c_str());
  //  curl_easy_setopt(curlSession, CURLOPT_NOBODY, 1L);//不获取文件本身
  //  curl_easy_setopt(curlSession, CURLOPT_FILETIME, 1L);//获得文件时间
  curl_easy_setopt(curlSession, CURLOPT_WRITEFUNCTION,
                   &ftpSession::writeStringCallbask);
  curl_easy_setopt(curlSession, CURLOPT_WRITEDATA, &folderList);
  //  curl_easy_setopt(curlSession, CURLOPT_HEADER, 0L);

  std::vector<oFileInfo> listInfo;
  CURLcode err = static_cast<CURLcode>(perform());
  if (err != CURLE_OK) {
    DOODLE_LOG_WARN << curl_easy_strerror(err);
    return listInfo;
  }
  QStringList::const_iterator iter_folder;

  QStringList list_folder = folderList.split("\r\n", QString::SkipEmptyParts);
  for (iter_folder = list_folder.begin(); iter_folder != list_folder.end();
       iter_folder++) {
    oFileInfo info;
    info.isFolder = false;
    QStringList folderInfo = iter_folder->split(" ", QString::SkipEmptyParts);
    if (folderInfo[0].at(0) == "d") info.isFolder = true;

    // folderInfo[folderInfo.size() - 1]
    QStringList p(remote);
    p.prepend("");
    p.append(folderInfo[folderInfo.size() - 1]);
    info.filepath = p.join("/").toStdString();

    if (info.isFolder) info.filepath += "/";
    listInfo.push_back(info);
  }
  return listInfo;
}

size_t ftpSession::writeFileCallbask(void *buff, size_t size, size_t nmemb,
                                     void *data) {
  if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1) || (data == nullptr))
    return 0;
  auto *file = static_cast<QFile *>(data);
  if (file->isOpen()) {
    file->write(reinterpret_cast<char *>(buff), size * nmemb);
  }
  return size * nmemb;
}

size_t ftpSession::readFileCallbask(void *buff, size_t size, size_t nmemb,
                                    void *data) {
  auto *file = static_cast<QFile *>(data);
  return file->read(reinterpret_cast<char *>(buff), size * nmemb);
}

size_t ftpSession::notCallbask(void *buff, size_t size, size_t nmemb,
                               void *data) {
  return size * nmemb;
}
size_t ftpSession::writeStringCallbask(void *ptr, size_t size, size_t nmemb,
                                       void *data) {
  if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1) || (data == nullptr))
    return 0;
  auto *ptrlist = static_cast<QString *>(data);
  if (ptrlist != nullptr) {
    ptrlist->append(QByteArray(reinterpret_cast<char *>(ptr), boost::numeric_cast<int>(size * nmemb)));
    return size * nmemb;
  }
  return 0;
}

int ftpSession::perform() {
  CURLcode err;
  err = curl_easy_perform(curlSession);
  if (err != CURLE_OK) DOODLE_LOG_WARN << curl_easy_strerror(err);
  return err;
}

bool ftpSession::uploadFolder(const dstring &localFolder,
                              const dstring &remoteFolder) {
  bool err = true;
  auto localFile_ =
      QDir::cleanPath(QString::fromStdString(localFolder));  //清理路径多余字符
  auto remoteFile_ = QDir::cleanPath(QString::fromStdString(remoteFolder));

  if (QFileInfo(localFile_).isFile()) {
    err &= upload(localFile_.toStdString(), remoteFile_.toStdString());
  } else {
    auto k_local_iter =
        new QDirIterator(localFile_, QDirIterator::Subdirectories);
    while (k_local_iter->hasNext()) {
      auto file = k_local_iter->filePath();
      if (QFileInfo(file).isFile()) {
        auto rem_file = file;
        err &= upload(file.toStdString(),
                      rem_file.replace(localFile_, remoteFile_).toStdString());
      }
      k_local_iter->next();
    }
  }
  return err;
}
bool ftpSession::downFolder(const dstring &localFile,
                            const dstring &remoteFile) {
  bool err = true;
  auto localFile_ =
      QDir::cleanPath(QString::fromStdString(localFile));  //清理路径多余字符
  auto remoteFile_ = QDir::cleanPath(QString::fromStdString(remoteFile));

  auto k_lo_dir = QDir(localFile_);
  if (!k_lo_dir.exists()) k_lo_dir.mkpath(QDir::cleanPath(localFile_));

  auto k_list = list((remoteFile_ + "/").toStdString());
  for (auto &&k_f : k_list) {
    //文件夹的话直接递归
    auto k_loca_path = QString::fromStdString(k_f.filepath);
    if (k_f.isFolder) {
      err &=
          downFolder(k_loca_path.replace(remoteFile_, localFile_).toStdString(),
                     k_f.filepath);
    } else {
      err &= down(k_loca_path.replace(remoteFile_, localFile_).toStdString(),
                  k_f.filepath);
    }
  }
  return err;
}
bool ftpSession::rename(const dstring &oldName, const dstring &newName) {
  ptrUrl->setPath("");
  curl_easy_reset(curlSession);
  // ftp命令
  struct curl_slist *ftpList = nullptr;
  boost::format str("%s %s");
  str % "RNFR" % oldName;
  DOODLE_LOG_INFO << str.str().c_str();
  ftpList = curl_slist_append(ftpList, str.str().c_str());

  str.clear();
  str % "RNTO" % newName;
  DOODLE_LOG_INFO << str.str().c_str();
  ftpList = curl_slist_append(ftpList, str.str().c_str());

  //创建上传设置
  curl_easy_setopt(curlSession, CURLOPT_URL,
                   ptrUrl->toString().toStdString().c_str());
  curl_easy_setopt(curlSession, CURLOPT_NOBODY, 1L);
  curl_easy_setopt(curlSession, CURLOPT_POSTQUOTE, ftpList);

  CURLcode err = static_cast<CURLcode>(perform());
  auto isOK = false;
  if (err != CURLE_OK) {
    isOK = false;
  }
  curl_slist_free_all(ftpList);
  isOK = true;
  return isOK;
}
bool ftpSession::upload(const dstring &localFile, const dstring &remoteFile,
                        bool backupFile) {
  boost::filesystem::path path(remoteFile);  // 2018-09-19 08:59:07
  QDateTime dt = QDateTime::currentDateTime();
  //每小时一个文件
  QString fileNameDt = dt.toString("yyyy_MM_dd_hh_mm");
  path = path.parent_path() / "backup" / fileNameDt.toStdString() /
         path.filename();

  createDir(path.parent_path().generic_string());
  if (backupFile) {
    rename(remoteFile, path.generic_string());
  }
  return upload(localFile, remoteFile);
}

DSYSTEM_E
