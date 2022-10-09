//
// Created by TD on 2022/5/17.
//

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_core/json_rpc/json_rpc_i.h>
namespace doodle {
class DOODLELIB_API json_rpc_server : public json_rpc_server_i {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  json_rpc_server();
  ~json_rpc_server() override;

  entt::entity create_movie(
      const create_move_arg& in_arg
  ) override;
  process_message get_progress(entt::entity in_id) override;
  void stop_app() override;
};
}  // namespace doodle
