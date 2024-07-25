#include <doodle_core/metadata/computer.h>
#include <doodle_lib/core/http/http_session_data.h>
namespace doodle {
class server_task_info;
}

namespace doodle::http {

class computer_reg_data {
 public:
  computer_reg_data() = default;
  explicit computer_reg_data(const doodle::computer& in_data) : computer_data_(in_data) {}
  ~computer_reg_data() = default;

  doodle::computer computer_data_;
  std::shared_ptr<doodle::server_task_info> task_info_;
  entt::entity task_info_entity_{};
  std::weak_ptr<detail::http_websocket_data> websocket_data_;
};
using computer_reg_data_ptr = std::shared_ptr<computer_reg_data>;
class computer_reg_data_manager {
  std::set<computer_reg_data_ptr> computer_reg_datas_;
  std::mutex mutex_;
  void clear_old();

 public:
  computer_reg_data_manager()  = default;
  ~computer_reg_data_manager() = default;

  void reg(computer_reg_data_ptr in_data);
  std::set<computer_reg_data_ptr> list();
  static computer_reg_data_manager& get();
};
}  // namespace doodle::http