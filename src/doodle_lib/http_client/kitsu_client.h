#pragma once

#include "doodle_core/metadata/project.h"
#include <doodle_core/core/http_client_core.h>
#include <doodle_core/metadata/assets.h>

#include <doodle_lib/core/http/json_body.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio/awaitable.hpp>

#include <filesystem>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <tl/expected.hpp>

namespace doodle {
class async_task;
}

namespace doodle::kitsu {

class kitsu_client {
  using http_client_t     = doodle::http::http_client;
  using http_client_ptr_t = std::shared_ptr<http_client_t>;
  http_client_ptr_t http_client_ptr_{};

 public:
  explicit kitsu_client(const std::string& kitsu_url) : http_client_ptr_{std::make_shared<http_client_t>(kitsu_url)} {}
  template <typename ExecutorType>
  explicit kitsu_client(const std::string& kitsu_url, ExecutorType&& in_executor)
      : http_client_ptr_{std::make_shared<http_client_t>(kitsu_url, std::forward<ExecutorType>(in_executor))} {}
  struct file_association {
    FSys::path ue_file_;
    FSys::path maya_file_;
    details::assets_type_enum type_;
  };
  boost::asio::awaitable<file_association> get_file_association(const uuid& in_task_id) const;

  boost::asio::awaitable<FSys::path> get_ue_plugin(const std::string& in_version) const;

  boost::asio::awaitable<FSys::path> get_task_maya_file(const uuid& in_task_id) const;

  boost::asio::awaitable<project> get_project(const uuid& in_project_id) const;
  boost::asio::awaitable<std::shared_ptr<async_task>> get_generate_uesk_file_arg(const uuid& in_task_id) const;
};

}  // namespace doodle::kitsu