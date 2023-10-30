//
// Created by td_main on 2023/10/19.
//

#pragma once
#include <doodle_core/core/file_sys.h>
#include <doodle_core/metadata/project.h>

#include <boost/asio.hpp>

#include <entt/entt.hpp>
namespace doodle::http {

class project_storage_type {
 public:
  using executor_type = decltype(boost::asio::make_strand(g_io_context()));

  project_storage_type();
  explicit project_storage_type(FSys::path in_project_path);

  entt::registry& get_registry() { return *registry_; }

  void load_project();

  void save_project();
  inline executor_type get_executor() const noexcept { return executor_; };

 private:
  registry_ptr registry_;
  std::string project_name_;
  FSys::path project_path_;
  std::any loader_;
  executor_type executor_;
};

class project_storage_list_type {
 public:
  project_storage_list_type() = default;
  FSys::path project_root{};
  std::map<std::string, project_storage_type> project_list_{};
};
}  // namespace doodle::http