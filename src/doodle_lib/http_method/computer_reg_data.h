#include <doodle_core/metadata/computer.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/http_websocket_client.h>
namespace doodle {
class server_task_info;
}

namespace doodle::http {

class computer_reg_data {
  friend class computer_reg_data_manager;

 public:
  computer_reg_data() : computer_data_ptr_(std::make_shared<doodle::computer>()) {};
  ~computer_reg_data() = default;

  std::shared_ptr<doodle::computer> computer_data_ptr_;
  std::shared_ptr<doodle::server_task_info> task_info_;
  std::weak_ptr<http_websocket_client> client;
};
using computer_reg_data_ptr      = std::shared_ptr<computer_reg_data>;
using computer_reg_data_weak_ptr = std::weak_ptr<computer_reg_data>;
class computer_reg_data_manager {
  std::vector<computer_reg_data_weak_ptr> computer_reg_datas_;
  std::mutex mutex_;
  void clear_old();

 public:
  computer_reg_data_manager()  = default;
  ~computer_reg_data_manager() = default;

  void reg(computer_reg_data_ptr in_data);
  std::vector<computer_reg_data_ptr> list();
  void clear(const computer_reg_data_ptr& in_data);
  static computer_reg_data_manager& get();
};
}  // namespace doodle::http