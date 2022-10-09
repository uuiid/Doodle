//
// Created by TD on 2022/5/9.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/json_rpc/core/rpc_server.h>
#include <doodle_core/json_rpc/args/rpc_json_progress.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/move_create.h>
#include <doodle_core/thread_pool/process_message.h>

#include <boost/signals2.hpp>
namespace doodle {

class DOODLE_CORE_API json_rpc_i {
 public:
  using json_rpc::rpc_server::rpc_server;

  class DOODLE_CORE_API create_move_arg {
   public:
    FSys::path out_path;
    std::vector<movie::image_attr> image;
    friend void DOODLE_CORE_API to_json(nlohmann::json& nlohmann_json_j, const create_move_arg& nlohmann_json_t) {
      nlohmann_json_j["out_path"] = nlohmann_json_t.out_path;
      nlohmann_json_j["image"]    = nlohmann_json_t.image;
    }
    friend void DOODLE_CORE_API from_json(const nlohmann::json& nlohmann_json_j, create_move_arg& nlohmann_json_t) {
      nlohmann_json_j.at("out_path").get_to(nlohmann_json_t.out_path);
      nlohmann_json_j.at("image").get_to(nlohmann_json_t.image);
    }
  };

 public:
  //  void init_register() override;

  virtual entt::entity create_movie(
      const create_move_arg& in_arg
  )                                                        = 0;

  virtual process_message get_progress(entt::entity in_id) = 0;
  virtual void stop_app()                                  = 0;
};

}  // namespace doodle
