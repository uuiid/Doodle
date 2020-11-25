#pragma once

#include <fileSystem_global.h>

#include "DfileSyntem.h"

#include <curl/curl.h>
#include <boost/property_tree/ptree.hpp>
class QFile;
class QUrl;

DSYSTEM_S

struct oFileInfo {
  dstring filepath;
  bool isFolder;
  time_t fileMtime;
  double fileSize;
};

class DSYSTEM_API ftpSession {

 public:
  ~ftpSession();

  bool down(const dstring &localFile, const dstring &remoteFile);
  bool downFolder(const dstring &localFile, const dstring &remoteFile);
  bool upload(const dstring &localFile, const dstring &remoteFile);
  bool upload(const dstring &localFile, const dstring &remoteFile,bool backupFile);
  bool uploadFolder(const dstring &localFolder, const dstring &remoteFolder);
  bool rename(const dstring &oldName, const dstring &newName);
  oFileInfo fileInfo(const dstring &remoteFile);
  //获得文件列表
  std::vector<oFileInfo> list(const dstring &remoteFolder);
  bool createDir(const dstring &path);
  bool createDir(const std::vector<dstring> &path, bool allPath);
  friend ftpSessionPtr DfileSyntem::session(const dstring &host,
                                            int prot,
                                            const dstring &name,
                                            const dstring &password);

 private:
  ftpSession();
  void setInfo(const dstring &host,
               int prot,
               const dstring &name,
               const dstring &password);
  static size_t writeFileCallbask(void *buff, size_t size, size_t nmemb, void *data);
  static size_t readFileCallbask(void *buff, size_t size, size_t nmemb, void *data);
  static size_t notCallbask(void *buff, size_t size, size_t nmemb, void *data);
  static size_t writeStringCallbask(void *ptr, size_t size, size_t nmemb, void *data);
  CURLcode perform();

 private:
  std::shared_ptr<QFile> outfile;
  std::shared_ptr<QFile> inputfile;
  std::shared_ptr<QUrl> ptrUrl;

  mutable CURL *curlSession;
};

inline bool ftpSession::createDir(const dstring &path) {
  return createDir(std::vector<dstring>{path}, true);
}
DSYSTEM_E
