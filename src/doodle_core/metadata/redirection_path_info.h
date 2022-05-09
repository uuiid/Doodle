//
// Created by TD on 2022/5/9.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
namespace doodle {

class DOODLE_CORE_EXPORT redirection_path_info {
 public:
  explicit redirection_path_info(
      std::string in_token,
      std::vector<FSys::path> in_search_path,
      FSys::path in_file_name);
  std::string token;
  std::vector<FSys::path> search_path;
  FSys::path file_name;
};
}  // namespace doodle
