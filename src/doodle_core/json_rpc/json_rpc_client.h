//
// Created by TD on 2022/5/9.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_core/json_rpc/core/rpc_client.h>
#include <doodle_core/json_rpc/args/rpc_json_progress.h>
namespace doodle {

class DOODLE_CORE_EXPORT json_rpc_client : public json_rpc::rpc_client {
 public:
  json_rpc_client(boost::asio::io_context& in_context, const std::string& in_host, uint16_t in_post);
  using image_to_move_arg = boost::coroutines2::coroutine<json_rpc::args::rpc_json_progress>;

  void image_to_move(image_to_move_arg::push_type& in_skin,
                     const std::vector<movie::image_attr>& in_list);

 public:
};

}  // namespace doodle
