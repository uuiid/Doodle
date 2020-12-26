#include "pathParsing.h"

#include <src/core/coreset.h>

#include <src/DfileSyntem.h>
#include <Logger.h>
#include <nlohmann/json.hpp>
#include <boost/filesystem.hpp>

#include <sstream>

CORE_NAMESPACE_S
pathParsing::pathParsing()
    : p_path_row() {
}

// dpathList pathParsing::getPath() {
//   dpathList list{};
//   try {
//     //如果是pathjson流就直接解析
//     auto path = nlohmann::json::parse(p_path_row.generic_string());
//     for (auto&& i : path) {
//       if (i.is_string())
//         list.push_back(i.get<std::string>());
//     }

//   } catch (const nlohmann::json::parse_error& err) {
//     DOODLE_LOG_INFO(err.what());
//     auto& fileSys = doSystem::DfileSyntem::get();

//     //先判断存在性
//     if (fileSys.exists(p_path_row)) {
//       if (p_path_row.extension() == ".json") {
//         auto file = fileSys.open(p_path_row, std::ios_base::in);
//         nlohmann::json root{};
//         (*file) >> root;
//         for (auto&& i : root) {
//           if (i.is_string())
//             list.push_back(i.get<std::string>());
//         }
//       }
//     }
//   }
//   return list;
// }

dpathList pathParsing::getPath(const std::string& pathstr) {
  dpathList list{};

  dpath path{pathstr};
  if (boost::filesystem::portable_name(pathstr) && path.extension() != ".json") {
    list.push_back(pathstr);
  } else {
    try {
      //如果是pathjson流就直接解析
      auto root = nlohmann::json::parse(pathstr);
      for (auto&& i : root) {
        if (i.is_string())
          list.push_back(i.get<std::string>());
      }

    } catch (const nlohmann::json::parse_error& err) {
      DOODLE_LOG_INFO(err.what());
      auto& fileSys = doSystem::DfileSyntem::get();

      //先判断存在性
      if (fileSys.exists(path)) {
        if (path.extension() == ".json") {
          auto file = fileSys.open(path, std::ios_base::in);

          nlohmann::json root{};
          try {
            (*file) >> root;
            for (auto&& i : root) {
              if (i.is_string())
                list.push_back(i.get<std::string>());
            }
            if (list.empty()) {
              list.push_back(pathstr);
            }
            file->close();
          } catch (const nlohmann::json::parse_error& err) {
            DOODLE_LOG_INFO(err.what());
            list.push_back(pathstr);
          }
        } else {
          list.push_back(pathstr);
        }
      } else {
        list.push_back(pathstr);
      }
    }
  }

  return list;
}

void pathParsing::write(const dpathList& path_value) {
  nlohmann::json root;
  for (auto&& path : path_value) {
    root.push_back(path.generic_string());
  }
  auto file = doSystem::DfileSyntem::get().open(p_path_row, std::ios_base::out);

  (*file) << root.dump();
}

void pathParsing::write(const dpathList& path_value, const dpath filePath) {
  p_path_row = filePath;
  write(path_value);
}

dpathList pathParsing::operator()() {
  return dpathList();
}

CORE_NAMESPACE_E