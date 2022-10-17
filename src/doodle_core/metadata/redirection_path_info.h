//
// Created by TD on 2022/5/9.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
namespace doodle {
class redirection_path_info;

class DOODLE_CORE_API redirection_path_info {
 public:
  redirection_path_info();
  explicit redirection_path_info(

      std::vector<FSys::path> in_search_path, FSys::path in_file_name
  );

  std::vector<FSys::path> search_path_;
  FSys::path file_name_;

  [[nodiscard]] std::optional<FSys::path> get_replace_path() const;

 private:
  friend void DOODLE_CORE_API to_json(nlohmann::json& j, const redirection_path_info& p);
  friend void DOODLE_CORE_API from_json(const nlohmann::json& j, redirection_path_info& p);
};

}  // namespace doodle
