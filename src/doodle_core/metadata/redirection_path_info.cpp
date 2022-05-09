//
// Created by TD on 2022/5/9.
//

#include "redirection_path_info.h"
namespace doodle {
void to_json(nlohmann::json& j, const redirection_path_info& p) {
  j["token"]       = p.token_;
  j["search_path"] = p.search_path_;
  j["file_name"]   = p.file_name_;
}
void from_json(const nlohmann::json& j, redirection_path_info& p) {
  j.at("token").get_to(p.token_);
  j.at("search_path").get_to(p.search_path_);
  j.at("file_name").get_to(p.file_name_);
}
redirection_path_info::redirection_path_info() = default;
redirection_path_info::redirection_path_info(
    std::string in_token,
    std::vector<FSys::path> in_search_path,
    std::filesystem::path in_file_name)
    : token_(std::move(in_token)),
      search_path_(std::move(in_search_path)),
      file_name_(std::move(in_file_name)) {
}
}  // namespace doodle
