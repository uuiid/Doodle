//
// Created by TD on 25-3-6.
//

#include "http_jwt_fun.h"

#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/seedance2/subproject.h"
#include <doodle_core/metadata/person.h>
#include <doodle_core/metadata/task.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>

#include <doodle_lib/core/global_function.h>
#include <doodle_lib/http_client/url_sign.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "sqlite_orm/sqlite_select_data.h"
#include <jwt-cpp/traits/nlohmann-json/traits.h>

namespace doodle::http {

bool http_jwt_or_signed_fun::try_signed_url(const session_data_ptr& in_handle) {
  // 签名只授权「读」, 所以仅 GET; POST/DELETE 等一律走会话鉴权
  if (in_handle->method() != boost::beast::http::verb::get) return false;
  // 请求带了会话凭据(cookie 里的 access_token_cookie / Authorization 的 Bearer)**就不再兜底到签名**:
  // 身份与结论一律交给会话路径(有效则通过, 过期或冻结则 401), 免得 URL 里的 uid 盖掉登录态的身份。
  // 判断看的是「凭据本身」而不是 cookie 头是否为空 —— 一个无关 cookie 不该让第三方签名请求失去兜底。
  // 检测口径与 http_jwt_fun::parse_header 的取法保持一致(cookie 在前, Bearer 在后)。
  const std::string l_cookie{in_handle->req_header_[boost::beast::http::field::cookie]};
  const std::string l_auth{in_handle->req_header_[boost::beast::http::field::authorization]};
  if (l_cookie.find("access_token_cookie=") != std::string::npos || l_auth.starts_with("Bearer ")) return false;
  if (!g_ctx().contains<kitsu_ctx_t>()) return false;

  auto&& l_params = in_handle->url_.params();
  auto l_exp_it   = l_params.find(url_sign_t::g_exp_key);
  auto l_sig_it   = l_params.find(url_sign_t::g_sig_key);
  if (l_exp_it == l_params.end() || l_sig_it == l_params.end()) return false;
  if (!(*l_exp_it)->has_value || !(*l_sig_it)->has_value) return false;

  // 待签名的路径部分必须与签名时**逐字节**一致: 用 encoded_path() 而不是 path(),
  // 后者会做百分号解码(".../Program%20Files/..." -> ".../Program Files/..."), 于是含转义的
  // 路径会与签名串对不上, 表现成一律 401。
  auto l_path_view = in_handle->url_.encoded_path();
  const std::string l_path{l_path_view.data(), l_path_view.size()};
  const std::string l_exp{(*l_exp_it)->value};
  const std::string l_sig{(*l_sig_it)->value};
  // uid 可选: 没有人员上下文的调用方可以不传
  auto l_uid_it = l_params.find(url_sign_t::g_uid_key);
  const std::string l_uid{
      (l_uid_it != l_params.end() && (*l_uid_it)->has_value) ? std::string{(*l_uid_it)->value} : std::string{}
  };

  std::string l_reason{};
  if (!url_sign_t::verify(
          l_path, l_exp, l_uid, l_sig, g_ctx().get<kitsu_ctx_t>().secret_, url_sign_t::now_seconds(), &l_reason
      )) {
    SPDLOG_LOGGER_DEBUG(g_logger_ctrl().get_http(), "签名 URL 校验失败 path [{}]: {}", l_path, l_reason);
    return false;
  }

  // 签名合法 = 凭据合法, 但授权校验仍在 handler 里按人员身份照常执行
  if (!l_uid.empty()) load_person(from_uuid_str(l_uid));

  SPDLOG_LOGGER_TRACE(g_logger_ctrl().get_http(), "签名 URL 通过 path [{}] uid [{}]", l_path, l_uid);
  return true;
}

void http_jwt_or_signed_fun::parse_header(const session_data_ptr& in_handle) {
  // 优先级: **先 cookie / Bearer**(与 http_jwt_fun 完全一致) -> 再短时效签名 URL。
  //
  // 顺序不能反: 会话凭据里的身份来自登录态, 而 uid 来自 URL。若先看签名, 一个已登录的
  // 管理员点开别人签发的图片 URL 时, 就会以 URL 里的 uid 去做授权校验(被别人降权);
  // 先会话则始终以「当前登录的人」为准。
  //
  // 这条顺序在 try_signed_url 里还有第二道保险: 请求一旦带了会话凭据, 签名兜底直接不参与
  // (即便会话凭据已过期, 也按会话路径报 401, 不会静默降级成签名身份)。
  if (try_signed_url(in_handle)) return;
  // 两条路都不通: 再走一次会话路径, 把统一的 401("请先登录" / "cookie 验证错误")抛出去
  http_jwt_fun::parse_header(in_handle);
}

void http_jwt_fun::parse_header(const session_data_ptr& in_handle) {
  if (in_handle->method() == boost::beast::http::verb::options) return;
  if (!g_ctx().contains<kitsu_ctx_t>()) return;
  auto l_jwt = in_handle->req_header_[boost::beast::http::field::cookie];
  if (l_jwt.empty()) l_jwt = in_handle->req_header_[boost::beast::http::field::authorization];

  if (auto l_it = l_jwt.find("access_token_cookie="); l_it != std::string::npos)
    l_jwt = l_jwt.substr(l_it + 20, l_jwt.find(';', l_it) - l_it - 20);
  else if (auto l_it_b = l_jwt.find("Bearer "); l_it_b != std::string::npos)
    l_jwt = l_jwt.substr(l_it_b + 7, l_jwt.find(' ', l_it_b) - l_it_b - 7);
  else if (
      auto l_it1 = in_handle->url_.params().find("access_token_cookie");
      l_it1 != in_handle->url_.params().end() && (*l_it1)->has_value
  )
    l_jwt = (*l_it1)->value;
  else if (
      auto l_it2 = in_handle->url_.params().find("access_token");
      l_it2 != in_handle->url_.params().end() && (*l_it2)->has_value
  )
    l_jwt = (*l_it2)->value;
  if (l_jwt.empty()) throw_exception(http_request_error{boost::beast::http::status::unauthorized, "请先登录"});
  // std::string l_l_jwt_str{l_jwt};

  auto& l_ctx       = g_ctx().get<kitsu_ctx_t>();

  auto verifier     = jwt::verify().allow_algorithm(jwt::algorithm::hs256{l_ctx.secret_});
  auto l_jwt_decode = jwt::decode(l_jwt);
  try {
    verifier.verify(l_jwt_decode);
  } catch (const std::system_error& e) {
    throw_exception(
        http_request_error{boost::beast::http::status::unauthorized, fmt::format("cookie 验证错误 {}", e.what())}
    );
  }
  auto l_uuid = from_uuid_str(l_jwt_decode.get_subject());
  load_person(l_uuid);
}

void http_jwt_fun::load_person(const uuid& in_uuid) {
  auto l_sql = get_sqlite_database();
  // default_logger_raw()->warn("{}", in_uuid);
  if (l_sql.uuid_to_id<person>(in_uuid) == 0)
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "请先注册"});
  {
    using namespace orm;
    auto l_person =
        select(l_sql).columns(object<person>()).from<person>().where(c(&person::uuid_id_) == in_uuid)().to_single();
    l_person.departments_ = select(l_sql)
                                .columns(&person_department_link::department_id_)
                                .from<person_department_link>()
                                .where(c(&person_department_link::person_id_) == l_person.uuid_id_)()
                                .to_vector();
    person_               = {l_person};
  }

  if (!person_.person_.active_)
    throw_exception(
        http_request_error{
            boost::beast::http::status::unauthorized, fmt::format("账户 {} 已被冻结", person_.person_.email_)
        }
    );
}

void http_jwt_fun::http_jwt_t::check_project_manager(const uuid& in_project_id) const {
  if (person_.uuid_id_.is_nil())
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
  if (!is_project_manager(in_project_id))
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
}
bool http_jwt_fun::http_jwt_t::is_project_manager(const uuid& in_project_id) const {
  return person_.role_ == person_role_type::admin ||                            // 是管理员
         person_.role_ == person_role_type::producer ||                         //  是制片
         (person_.role_ == person_role_type::manager &&                         //
          get_sqlite_database().is_person_in_project(person_, in_project_id));  // 是项目经理并且在项目中
}

void http_jwt_fun::http_jwt_t::check_project_access(const uuid& in_project_id) const {
  if (person_.uuid_id_.is_nil())
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
  if (
      !(                                                                        //
          person_.role_ == person_role_type::admin ||                           // 是管理员
          person_.role_ == person_role_type::producer ||                        //  是制片
          person_.role_ == person_role_type::manager ||                         // 是项目经理
          (person_.role_ == person_role_type::supervisor &&                     // 是项目主管
           get_sqlite_database().is_person_in_project(person_, in_project_id))  // 在项目中
      )                                                                         //
  )
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
}

bool http_jwt_fun::http_jwt_t::is_admin() const {
  return !person_.uuid_id_.is_nil() && person_.role_ == person_role_type::admin;
}

bool http_jwt_fun::http_jwt_t::is_outsourcer() const {
  return !person_.uuid_id_.is_nil() && person_.role_ == person_role_type::outsource;
}
void http_jwt_fun::http_jwt_t::check_not_outsourcer() const {
  if (is_outsourcer()) {
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
  }
}

bool http_jwt_fun::http_jwt_t::is_in_project(const uuid& in_project_id) const {
  if (person_.uuid_id_.is_nil()) return false;
  return person_.role_ == person_role_type::admin || get_sqlite_database().is_person_in_project(person_, in_project_id);
}
void http_jwt_fun::http_jwt_t::check_in_project(const uuid& in_project_id) const {
  if (!is_in_project(in_project_id)) {
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
  }
}

void http_jwt_fun::http_jwt_t::check_admin() const {
  if (is_admin()) return;

  throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
}

bool http_jwt_fun::http_jwt_t::is_producer() const {
  return !person_.uuid_id_.is_nil() &&
         (person_.role_ == person_role_type::producer || person_.role_ == person_role_type::manager ||
          person_.role_ == person_role_type::admin);
}

void http_jwt_fun::http_jwt_t::check_producer() const {
  if (is_producer()) return;

  throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
}

void http_jwt_fun::http_jwt_t::check_manager() const {
  if (is_manager()) return;

  throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
}

bool http_jwt_fun::http_jwt_t::is_manager() const {
  return !person_.uuid_id_.is_nil() &&
         (person_.role_ == person_role_type::manager || person_.role_ == person_role_type::producer ||
          person_.role_ == person_role_type::admin);
}

bool http_jwt_fun::http_jwt_t::is_supervisor() const {
  return !person_.uuid_id_.is_nil() &&
         (person_.role_ == person_role_type::supervisor || person_.role_ == person_role_type::manager ||
          person_.role_ == person_role_type::producer || person_.role_ == person_role_type::admin);
}

void http_jwt_fun::http_jwt_t::check_supervisor() const {
  if (is_supervisor()) return;

  throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
}

void http_jwt_fun::http_jwt_t::check_user() const {
  if (!person_.uuid_id_.is_nil() && (person_.role_ == person_role_type::user || is_supervisor())) return;

  throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
}

void http_jwt_fun::http_jwt_t::check_project_supervisor(const uuid& in_project_id) const {
  if (is_project_supervisor(in_project_id)) return;
  throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
}
bool http_jwt_fun::http_jwt_t::is_project_supervisor(const uuid& in_project_id) const {
  return is_project_manager(in_project_id) || (person_.role_ == person_role_type::supervisor &&
                                               get_sqlite_database().is_person_in_project(person_, in_project_id));
}
void http_jwt_fun::http_jwt_t::check_task_action_access(const uuid& in_task_id) const {
  check_task_action_access(get_sqlite_database().get_by_uuid<task>(in_task_id));
}
void http_jwt_fun::http_jwt_t::check_task_status_access(const uuid& in_target_status_id) const {
  if (person_.role_ != person_role_type::user) return;
  auto l_sql   = get_sqlite_database();
  auto l_stats = l_sql.get_by_uuid<task_status>(in_target_status_id);
  if (l_stats.is_artist_allowed_) return;
  throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
}

void http_jwt_fun::http_jwt_t::check_task_action_access(const task& in_task_id) const {
  if (person_.role_ == person_role_type::admin) return;
  auto l_sql = get_sqlite_database();
  if (!l_sql.is_person_in_project(person_, in_task_id.project_id_))
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
  if (person_.role_ == person_role_type::manager || person_.role_ == person_role_type::producer ||
      person_.role_ == person_role_type::supervisor)
    return;

  // if (l_sql.is_task_assigned_to_person(in_task_id.uuid_id_, person_.uuid_id_)) return;
  // throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
}

void http_jwt_fun::http_jwt_t::check_task_department_access(const task& in_task_id, const person& in_person_id) const {
  if (is_task_department_access(in_task_id, in_person_id)) return;
  throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
}

bool http_jwt_fun::http_jwt_t::is_task_department_access(const task& in_task_id, const person& in_person_id) const {
  if (person_.role_ == person_role_type::admin || person_.role_ == person_role_type::producer ||
      person_.role_ == person_role_type::manager)
    return true;
  auto l_sql = get_sqlite_database();
  if (l_sql.is_person_in_project(person_, in_task_id.project_id_)) return true;

  auto l_task_type = l_sql.get_by_uuid<task_type>(in_task_id.task_type_id_);
  if (person_.role_ == person_role_type::supervisor) {
    if (person_.departments_.empty()) return false;
    if (std::ranges::find(person_.departments_, l_task_type.uuid_id_) != person_.departments_.end() &&
        !in_person_id.departments_.empty())
      return true;
  }
  if (std::ranges::find(in_person_id.departments_, l_task_type.uuid_id_) != in_person_id.departments_.end() &&
      person_.uuid_id_ == in_person_id.uuid_id_)
    return true;

  return false;
}

void http_jwt_fun::http_jwt_t::check_delete_access(const uuid& in_project_id) const {
  if (person_.role_ == person_role_type::admin || person_.role_ == person_role_type::producer ||
      person_.role_ == person_role_type::manager)
    return;
  auto l_sql = get_sqlite_database();
  if (!l_sql.is_person_in_project(person_, in_project_id))
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
}

uuid http_jwt_fun::http_jwt_t::get_ai_studio_id() const { return person_.ai_studio_id_; }

void http_jwt_fun::http_jwt_t::check_subproject_access(const uuid& in_subproject_id) const {
  if (!is_subproject_access(in_subproject_id))
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
}
bool http_jwt_fun::http_jwt_t::is_subproject_access(const uuid& in_subproject_id) const {
  if (person_.ai_studio_id_.is_nil()) return false;
  if (person_.role_ == person_role_type::admin || person_.role_ == person_role_type::producer ||
      person_.role_ == person_role_type::manager)
    return true;
  auto l_sql = get_sqlite_database();
  using namespace orm;
  using namespace doodle::seedance2;
  return select(l_sql)
      .columns(count(&subproject_person_link::id_))
      .from<subproject_person_link>()
      .where(
          c(&subproject_person_link::subproject_id_) == in_subproject_id &&
          c(&subproject_person_link::person_id_) == person_.uuid_id_
      )
      ()
      .to_single() > 0;
}

}  // namespace doodle::http