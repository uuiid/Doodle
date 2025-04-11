//
// Created by TD on 25-3-6.
//

#pragma once
#include <doodle_lib/core/http/http_function.h>

namespace doodle {
struct person;
}

namespace doodle::http {
class http_jwt_fun : public http_function {
 protected:
  std::shared_ptr<person> person_;

  void get_person(const session_data_ptr& in_data);
  // 检查人员是否有修改项目属性的权限
  void is_project_manager(const uuid& in_project_id) const;
  // 检查是否是 admin
  bool is_admin() const;

 public:
  using http_function::http_function;

  boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
};
}  // namespace doodle::http