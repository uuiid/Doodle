//
// Created by TD on 2022/5/9.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_core/json_rpc/core/rpc_client.h>
#include <doodle_core/json_rpc/args/rpc_json_progress.h>

#include <doodle_core/metadata/project.h>
namespace doodle {

class DOODLE_CORE_EXPORT json_rpc_client : public json_rpc::rpc_client {
 public:
  json_rpc_client(boost::asio::io_context& in_context, const std::string& in_host, std::uint16_t in_post);

  void image_to_move(const std::vector<movie::image_attr>& in_list);
  json_rpc::args::rpc_json_progress get_progress();

  void open_project(const FSys::path& in_path);
  project_config::base_config get_project_config();

 public:
};

}  // namespace doodle
