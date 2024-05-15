#include <doodle_core/metadata/computer.h>

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
};
}  // namespace doodle::http