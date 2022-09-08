//
// Created by TD on 2022/5/9.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/json_rpc/core/rpc_server.h>
#include <doodle_core/json_rpc/args/rpc_json_progress.h>
#include <doodle_core/metadata/project.h>

#include <boost/signals2.hpp>
namespace doodle {

class DOODLE_CORE_API json_rpc_server_i : public json_rpc::rpc_server {
 public:
  using json_rpc::rpc_server::rpc_server;

 public:
  void init_register() override;

  virtual json_rpc::args::rpc_json_progress create_movie(const std::vector<movie::image_attr>& in_arg) = 0;
};

}  // namespace doodle
