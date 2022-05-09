//
// Created by TD on 2022/5/9.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/json_rpc/core/rpc_server.h>
namespace doodle {

namespace json_rpc_server_ns {
constexpr const std::string_view open_project{"open_project"};
constexpr const std::string_view create_movie{"create_movie"};
constexpr const std::string_view get_create_movie_log{"get_create_movie_log"};
}  // namespace json_rpc_server_ns

class DOODLE_CORE_EXPORT json_rpc_server : public json_rpc::rpc_server {
 public:
  using json_rpc::rpc_server::rpc_server;

 public:
  virtual void init_register() override;

  virtual void open_project(const FSys::path& in_path) = 0;
  virtual void create_movie()                          = 0;
  virtual std::string get_create_movie_log()           = 0;
};

}  // namespace doodle
