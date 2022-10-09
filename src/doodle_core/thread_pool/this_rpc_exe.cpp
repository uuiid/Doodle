//
// Created by TD on 2022/10/9.
//

#include "this_rpc_exe.h"

#include <boost/process.hpp>
#include <doodle_core/core/core_set.h>
#include <doodle_core/json_rpc/json_rpc_client.h>
#include <doodle_core/platform/win/get_prot.h>

namespace doodle::detail {
class this_rpc_exe::impl {
 public:
  FSys::path this_exe_path;

  boost::process::child this_exe_proces;
  std::shared_ptr<json_rpc_client> rpc_child;
};

this_rpc_exe::this_rpc_exe()
    : ptr(std::make_unique<impl>()) {
  ptr->this_exe_path = core_set::get_set().program_location() / "DoodleExe.exe"s;
}
void this_rpc_exe::stop_exit() {
  if (ptr->this_exe_proces.valid() && ptr->rpc_child) {
    ptr->rpc_child->stop_app();
  }
}
entt::handle this_rpc_exe::create_move() {
  ptr->rpc_child->image_to_move();
}
void this_rpc_exe::create_rpc_child() {
  if (ptr->this_exe_proces.valid() && ptr->rpc_child)
    return;

  ptr->this_exe_proces = boost::process::child{
      boost::process::exe  = ptr->this_exe_path,
      boost::process::args = "--json_rpc"};

  ptr->rpc_child = std::make_shared<json_rpc_client>(
      g_io_context(),
      "127.0.0.1"s,
      win::get_tcp_port(ptr->this_exe_proces.id())
  );
}

this_rpc_exe::~this_rpc_exe() = default;
}  // namespace doodle::detail
