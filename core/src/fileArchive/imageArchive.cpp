#include "imageArchive.h"
#include <src/core/coreset.h>
#include <src/fileDBInfo/filesqlinfo.h>
#include <regex>

CORE_NAMESPACE_S
imageArchive::imageArchive(fileSqlInfoPtr f_ptr)
    : fileArchive() {
  p_db_data = std::move(f_ptr);
}

bool imageArchive::update(const dpathList &filelist) {
  static const std::regex re_image{R"(\.(jpe?g|png|tga))"};
  if (filelist.empty()) return false;
  dpathList list;
  if (filelist.size() == 1 &&
      boost::filesystem::is_directory(filelist.front())) {  //这是一个文件夹
    for (auto &&item : boost::filesystem::directory_iterator(filelist.front())) {
      if (std::regex_search(item.path().extension().generic_string(), re_image)) {
        list.push_back(item.path());
      }
    }
  } else {
  }
  if (!list.empty()) {
    return fileArchive::update(list);
  } else {
    return false;
  }
}

void imageArchive::insertDB() {
  p_db_data->setFileList(p_ServerPath);
  if (p_db_data->getInfoP().empty()) {
    p_db_data->setInfoP("图片文件");
  }
  if (p_db_data->isInsert()) {
    p_db_data->updateSQL();
  } else {
    p_db_data->insert();
  }
}

void imageArchive::_generateFilePath() {
  if (!p_soureFile.empty()) {
    if (isServerzinsideDir(p_soureFile.front().generic_wstring())) {
      for (auto &&item : p_soureFile) {
        auto str = coreSet::toIpPath(item.generic_string());
        p_ServerPath.push_back(str);
      }
      return;
    }

    for (auto &&k_path : p_soureFile) {
      p_ServerPath.push_back(p_db_data->generatePath("Scenefiles") / k_path.filename());
    }
  } else if (!p_db_data->getFileList().empty()) {
    for (auto &&k_path : p_db_data->getFileList()) {
      p_ServerPath.push_back(k_path);
    }
  }
}

CORE_NAMESPACE_E
