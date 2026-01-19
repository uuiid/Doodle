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
 public:
  struct http_jwt_t;

 protected:
  void parse_header(const session_data_ptr& in_handle) override;

 public:
  struct http_jwt_t {
    person person_;
    bool is_admin() const;

    // 检查人员是否有修改项目属性的权限
    void check_project_manager(const uuid& in_project_id) const;
    bool is_project_manager(const uuid& in_project_id) const;
    // 检查人员是否有访问项目的权限
    void check_project_access(const uuid& in_project_id) const;
    // 检查是否是 admin
    void check_admin() const;
    // 检查是否是项目经理
    void check_manager() const;
    void check_supervisor() const;
    // 确认是否是在项目中的 supervisor
    void check_project_supervisor(const uuid& in_project_id) const;
    bool is_project_supervisor(const uuid& in_project_id) const;

    // 检查是否可以进行任务操作(只要 supervisor manager user 求在团队中)
    void check_task_action_access(const uuid& in_task_id) const;
    void check_task_action_access(const task& in_task_id) const;
    /// 检查是否可以被更改为目标状态
    void check_task_status_access(const uuid& in_target_status_id) const;
    // 检查任务的部门访问权限
    void check_task_department_access(const task& in_task_id, const person& in_person_id) const;
    bool is_task_department_access(const task& in_task_id, const person& in_person_id) const;
    // 检查是否可以删除任务
    void check_delete_access(const uuid& in_project_id) const;
    // 检查任务的分配权限
    void check_task_assign_access(const uuid& in_project_id) const;
  };
  using http_function::http_function;

 protected:
  http_jwt_t person_{};
};
#define DOODLE_HTTP_JWT_FUN(fun_name) DOODLE_HTTP_FUN_C(fun_name, ::doodle::http::http_jwt_fun)

}  // namespace doodle::http