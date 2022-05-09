//
// Created by TD on 2022/5/9.
//

#include "redirection_path_info.h"
namespace doodle {

redirection_path_info::redirection_path_info(
    std::string in_token,
    std::vector<FSys::path> in_search_path,
    std::filesystem::path in_file_name)
    : token(std::move(in_token)),
      search_path(std::move(in_search_path)),
      file_name(std::move(in_file_name)) {
}
}  // namespace doodle
