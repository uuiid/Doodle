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
    const create_move_arg& in_arg
) {
  auto l_h = make_handle();
  l_h.emplace<episodes>().analysis(in_arg.out_path);
  l_h.emplace<shot>().analysis(in_arg.out_path);
  l_h.emplace<FSys::path>(in_arg.out_path);
  g_reg()->ctx().at<image_to_move>()->async_create_move(
      l_h, in_arg.image, []() {}
  );
  return json_rpc::args::rpc_json_progress{in_arg.out_path};
}

json_rpc_server::~json_rpc_server() = default;
}  // namespace doodle
