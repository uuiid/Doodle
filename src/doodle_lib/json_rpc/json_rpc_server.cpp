//
// Created by TD on 2022/5/17.
//

#include "json_rpc_server.h"
#include <doodle_core/metadata/project.h>
#include <long_task/image_to_move.h>

namespace doodle {

class json_rpc_server::impl {
 public:
  impl() = default;
};

json_rpc_server::json_rpc_server()
    : ptr(std::make_unique<impl>()) {
}

json_rpc::args::rpc_json_progress json_rpc_server::create_movie(
    const create_move_arg& in_arg
) {
  return json_rpc::args::rpc_json_progress{""s};
}

json_rpc_server::~json_rpc_server() = default;
}  // namespace doodle
