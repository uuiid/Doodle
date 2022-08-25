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
  using image_to_move_sig = boost::signals2::signal<void(const json_rpc::args::rpc_json_progress&)>;

  void image_to_move(const image_to_move_sig& in_skin, const std::vector<movie::image_attr>& in_list);

 public:
};

}  // namespace doodle
