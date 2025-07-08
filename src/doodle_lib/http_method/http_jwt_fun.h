//
// Created by TD on 25-3-6.
//

#pragma once
#include <doodle_core/metadata/person.h>

#include <doodle_lib/core/http/http_function.h>
namespace doodle {
struct comment;
struct person;
struct task;
}  // namespace doodle

namespace doodle::http {
class http_jwt_fun : public http_function {
 protected:
  struct http_jwt_t {
    person person_;
    // 检查人员是否有修改项目属性的权限
    void is_project_manager(const uuid& in_project_id) const;
    // 检查是否是 admin
    void is_admin() const;
    // 检查是否是项目经理
    void is_manager() const;
    // 检查是否可以进行任务操作
    void check_task_action_access(const uuid& in_task_id) const;
    void check_task_action_access(const task& in_task_id) const;
    // 检查任务的部门访问权限
    void check_task_department_access(const task& in_task_id, const person& in_person_id) const;
    // 检查是否可以删除任务
    void check_delete_access(const uuid& in_project_id) const;
  };

  std::shared_ptr<http_jwt_t> get_person(const session_data_ptr& in_data);

 public:
  using http_function::http_function;
};

template <typename Capture_T>
class http_jwt_fun_template : public http_jwt_fun {
  boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override {
    return callback_arg(in_handle, std::static_pointer_cast<Capture_T>(in_handle->capture_));
  }

 public:
  using http_jwt_fun::http_jwt_fun;
  virtual boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
      session_data_ptr in_handle, const std::shared_ptr<Capture_T>& in_arg
  ) = 0;
};

}  // namespace doodle::http