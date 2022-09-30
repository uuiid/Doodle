//
// Created by TD on 2022/5/9.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/json_rpc/core/rpc_server.h>
#include <doodle_core/json_rpc/args/rpc_json_progress.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/move_create.h>

#include <boost/signals2.hpp>
namespace doodle {

class DOODLE_CORE_API json_rpc_server_i : public json_rpc::rpc_server {
 public:
  using json_rpc::rpc_server::rpc_server;

  class DOODLE_CORE_API create_move_arg {
   public:
    FSys::path out_path;
    std::vector<movie::image_attr> image;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(
        create_move_arg,
        out_path,
        image
    );
  };

 public:
  void init_register() override;

  virtual json_rpc::args::rpc_json_progress create_movie(
      const create_move_arg& in_arg
  ) = 0;
};

}  // namespace doodle
