//
// Created by TD on 2022/5/17.
//

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_core/json_rpc/json_rpc_server_i.h>
namespace doodle {
class DOODLELIB_API json_rpc_server : public json_rpc_server_i {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  json_rpc_server();
  ~json_rpc_server() override;

  void create_movie(
      const create_move_arg& in_arg
  ) override;
};
}  // namespace doodle
