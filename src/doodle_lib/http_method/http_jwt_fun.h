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
class DOODLELIB_API http_jwt_fun : public http_function {
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
    // 是否是外包
    bool is_outsourcer() const;
    // 检查不是外包
    void check_not_outsourcer() const;
    // 检查是否在项目中
    void check_in_project(const uuid& in_project_id) const;
    bool is_in_project(const uuid& in_project_id) const;

    // 检查人员是否有访问项目的权限
    void check_project_access(const uuid& in_project_id) const;
    // 检查是否是 admin
    void check_admin() const;
    // 检查是否是制片
    void check_producer() const;
    bool is_producer() const;
    // 检查是否是组长
    void check_manager() const;
    bool is_manager() const;
    // is_supervisor
    bool is_supervisor() const;
    // 检查是否是项目人
    void check_supervisor() const;
    // 是否是组员
    void check_user() const;
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
    // 从人员获取 ai工作室的id
    uuid get_ai_studio_id() const;

    // 检查人员是否在子项目中
    void check_subproject_access(const uuid& in_subproject_id) const;
    bool is_subproject_access(const uuid& in_subproject_id) const;
  };
  using http_function::http_function;

 protected:
  http_jwt_t person_{};

  /// 按 uuid 加载人员(含部门链接与冻结检查), 会话鉴权与签名 URL 鉴权共用。
  /// 未注册或已冻结会抛 401。
  void load_person(const uuid& in_uuid);
};
#define DOODLE_HTTP_JWT_FUN(fun_name) DOODLE_HTTP_FUN_C(fun_name, ::doodle::http::http_jwt_fun)

/**
 * 会话鉴权 **或** 短时效签名 URL, 二选一通过。
 *
 * 用途: 需要把站点内的素材 URL 交给第三方(火山方舟)去取图的路由。第三方无法携带我们的
 * cookie, 而会话 JWT 长效且无吊销名单, 不能放进 URL, 所以这类路由额外接受
 * `?exp=<unix>&uid=<人员>&sig=<hmac>` 的签名(见 url_sign_t)。
 *
 * 优先级: **先 cookie / Bearer**(浏览器与客户端的正常路径, 行为与 http_jwt_fun 完全一致),
 * 都没有或都不通过时, 才尝试签名兜底; 签名只对 **GET** 生效。
 *
 * 「先会话、后签名」这个顺序是有意的, 不是随手写的: 会话凭据里的身份来自登录态, 而 uid 来自 URL。
 * 一个已登录的管理员点开别人签发的图片 URL 时, 应当仍然以**自己**的身份通过授权校验,
 * 而不是被 URL 里的 uid 降级成别人的权限。
 *
 * 同理, 请求里带会话凭据时签名兜底**根本不参与**(见 try_signed_url): 会话凭据过期/冻结就按会话
 * 路径报 401, 不会退回签名身份。
 *
 * uid 会**真正加载 person_**(与 cookie 路径同一套校验: 注册、冻结、部门链接), 所以派生
 * handler 里的 check_project_access 等授权校验在签名路径下照常生效 —— 签名换的是凭据,
 * 不是绕过授权。uid 为空时不加载 person_, 需要人员校验的 handler 会 fail-closed。
 */
class DOODLELIB_API http_jwt_or_signed_fun : public http_jwt_fun {
 public:
  using http_jwt_fun::http_jwt_fun;

 protected:
  void parse_header(const session_data_ptr& in_handle) override;

  /// 校验签名 URL; 命中且校验通过返回 true(并已按 uid 加载 person_)。
  /// 请求带会话凭据(cookie 的 access_token_cookie / Authorization 的 Bearer)时**直接返回 false**,
  /// 把结论让给会话路径: 会话凭据在场就不该由签名决定身份, 过期/冻结也照常 401。
  bool try_signed_url(const session_data_ptr& in_handle);
};
#define DOODLE_HTTP_SIGNED_FUN(fun_name) DOODLE_HTTP_FUN_C(fun_name, ::doodle::http::http_jwt_or_signed_fun)

}  // namespace doodle::http