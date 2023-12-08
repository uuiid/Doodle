//
// Created by TD on 2023/12/8.
//

#include <doodle_core_fwd.h>

#include <entt/entt.hpp>
namespace doodle {

///  这个类是对消息组件
class process_callback {
  entt::handle handle_{};

 public:
  virtual ~process_callback() = default;

  process_callback& set_handle(const entt::handle& in_handle);

  ///  这个函数是用来处理消息的

  void operator()(boost::system::error_code in_error_code) const;
};

}  // namespace doodle
