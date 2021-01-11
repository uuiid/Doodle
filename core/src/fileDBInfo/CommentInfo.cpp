#include "CommentInfo.h"

#include <Logger.h>
#include <src/core/coreset.h>
#include <src/DfileSyntem.h>
#include <nlohmann/json.hpp>
DOODLE_NAMESPACE_S
CommentInfo::CommentInfo()
    : p_path_row() {
}

std::vector<std::string> CommentInfo::Info(const std::string &pathStr) {
  std::vector<std::string> list{};
  dpath path{pathStr};
  if (boost::filesystem::portable_name(pathStr) && path.extension() != ".json") {
    list.push_back(pathStr);
  } else {
    auto &fileSys = doSystem::DfileSyntem::get();
    if (fileSys.exists(path)) {
      auto file = fileSys.open(path, std::ios_base::in);

      nlohmann::json root{};
      try {
        (*file) >> root;
        for (auto it : root) {
          list.push_back(it.get<std::string>());
        }

        if (list.empty()) {
          list.push_back("");
        }
      } catch (const nlohmann::json::parse_error &err) {
        list.push_back("");
        DOODLE_LOG_INFO(err.what());
      }
    } else {
      list.push_back("");
    }
  }
  return list;
}

void CommentInfo::write(const std::string &Info_value) {
}

void CommentInfo::write(const std::string &Info_value, const dpath &file_Path) {
}
DOODLE_NAMESPACE_E
