//
// Created by TD on 2023/12/20.
//

#pragma once
#include <doodle_core/core/file_sys.h>
#include <doodle_core/doodle_core.h>

#include <entt/entt.hpp>
#include <map>

namespace doodle::gui::details {

class scan_category_t {
 public:
  struct project_root_t {
    FSys::path path_;
    std::string name_;
  };
  std::map<uuid, entt::entity>* uuid_map_entt_;
  scan_category_t() : uuid_map_entt_{nullptr} {}
  virtual ~scan_category_t()                                                                               = default;
  virtual std::vector<entt::handle> scan(const project_root_t& in_root) const                              = 0;
  virtual std::vector<entt::handle> check_path(const project_root_t& in_root, entt::handle& in_path) const = 0;
};

}  // namespace doodle::gui::details