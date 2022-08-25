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

void json_rpc_server::create_movie(
    const image_to_move_sig& in_skin,
    const std::vector<movie::image_attr>& in_arg
) {
  in_skin(json_rpc::args::rpc_json_progress{"test 0.3"s, "test 0.3"s, 0.3});
  in_skin(json_rpc::args::rpc_json_progress{"test 0.4"s, "test 0.4"s, 0.4});
  in_skin(json_rpc::args::rpc_json_progress{"test 0.5"s, "test 0.5"s, 0.5});
  in_skin(json_rpc::args::rpc_json_progress{"test 0.6"s, "test 0.6"s, 0.6});
  in_skin(json_rpc::args::rpc_json_progress{"test 1.0"s, "test 1.0"s, 1.0});
}

json_rpc_server::~json_rpc_server() = default;
}  // namespace doodle
