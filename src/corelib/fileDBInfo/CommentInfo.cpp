#include "CommentInfo.h"
#include <corelib/fileDBInfo/filesqlinfo.h>
#include <corelib/Exception/Exception.h>

#include <boost/filesystem.hpp>

#include <loggerlib/Logger.h>
#include <corelib/core/coreset.h>
#include <corelib/filesystem/FileSystem.h>
#include <nlohmann/json.hpp>

DOODLE_NAMESPACE_S
CommentInfo::CommentInfo(fileSqlInfo *file)
    : p_file_Archive(file),
      p_path(std::make_shared<dpath>()),
      p_info_list() {
}

std::vector<std::string> CommentInfo::Info(const std::string &pathStr) {
  /**
   *情况一：
   **传入路径
   *情况二：
   **传入json
   *情况三：
   **传入字符串 
   */

  dpath path{pathStr};
  auto &fileSys = DfileSyntem::get();

  if (fileSys.exists(path) && path.extension() == ".json") {  //传入路径的情况
    auto file = fileSys.readFileToString(path);

    *p_path = path;
    nlohmann::json root{};

    try {
      root = nlohmann::json::parse(*file);
      for (auto it : root) {
        p_info_list.push_back(it.get<std::string>());
      }
    } catch (const nlohmann::json::parse_error &err) {
      DOODLE_LOG_INFO(err.what());
    }

  } else {  //传入字符串（json或者直接字符串的情况下）
    nlohmann::json root;

    try {
      root = nlohmann::json::parse(pathStr);
      for (auto it : root) {
        p_info_list.push_back(it.get<std::string>());
      }
    } catch (const nlohmann::json::parse_error &err) {
      p_info_list.push_back(pathStr);
      DOODLE_LOG_INFO(err.what());
    }
  }

  return p_info_list;
}

std::vector<std::string> CommentInfo::Info() const {
  return p_info_list;
}

void CommentInfo::setInfo(const std::string &value) {
  p_info_list.push_back(value);
}

void CommentInfo::write() {
  nlohmann::json root;
  for (auto &&value : p_info_list) {
    root.push_back(value);
  }

  try {
    *p_path = p_file_Archive->generatePath("doodle", ".json", "info");

  } catch (const nullptr_error &err) {
    DOODLE_LOG_ERROR(err.what());
  }
  auto strPtr = std::make_shared<std::string>();
  strPtr->append(root.dump());
  DfileSyntem::get().writeFile(*p_path, strPtr);
}

std::string CommentInfo::DBInfo() const {
  if (!p_path->empty()) {
    return p_path->generic_string();
  } else {
    throw nullptr_error{"path is empty"};
  }
}

void CommentInfo::setFileSql(fileSqlInfo *file) {
  p_file_Archive = file;
}

DOODLE_NAMESPACE_E
