//
// Created by TD on 24-8-6.
//

#include "model_base.h"

#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/database_task/details/load_save_impl.h>
#include <doodle_core/database_task/details/project_config.h>
#include <doodle_core/database_task/details/tool.h>

#include <doodle_lib/core/http/http_function.h>

#include <treehh/tree.hh>

namespace nlohmann {
template <typename T>
struct [[maybe_unused]] adl_serializer<tree<T>> {
  using tree_t = tree<T>;

  static void iter_tree(json& j, const typename tree_t::iterator& in_tree_t) {
    for (auto l_it = std::begin(in_tree_t); l_it != std::end(in_tree_t); ++l_it) {
      j.emplace_back(*l_it);
      iter_tree(j, l_it);
    }
  }

  static void to_json(json& j, const tree_t& in_tree_t) { iter_tree(j, in_tree_t.begin()); }
  static void from_json(const json& j, tree_t& in_tree_t) {}
};
}  // namespace nlohmann

namespace doodle::http {
namespace {

tree<assets> get_trees(const entt::registry& in_reg) {
  auto l_view = in_reg.view<assets>();
  tree<assets> l_e{};

  std::function<void(const entt::const_handle&, const tree<assets>::iterator&)> l_buill_tree{};
  l_buill_tree = [&](const entt::const_handle& in_h, const tree<assets>::iterator& in_parent) {
    auto& l_ass = in_h.get<assets>();
    auto l_it   = l_e.append_child(in_parent, l_ass);
    for (auto&& l_c : l_ass.get_child()) {
      l_buill_tree(l_c, l_it);
    }
  };

  for (auto [e, l_ass] : l_view.each()) {
    if (!l_ass.get_parent()) {
      l_buill_tree(entt::const_handle{in_reg, e}, l_e.begin());
    }
  }

  return l_e;
}
}  // namespace

boost::asio::awaitable<boost::beast::http::message_generator> get_asset_tree(session_data_ptr in_handle) {
  co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "not found");
}

// 获取模型库基本的上下文
boost::asio::awaitable<boost::beast::http::message_generator> model_base_ctx_get(session_data_ptr in_handle) {
  auto l_this_exe = co_await boost::asio::this_coro::executor;
  co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
  auto&& l_prj = g_reg()->ctx().get<project_config::base_config>();
  nlohmann::json l_json;
  l_json["project"]        = g_reg()->ctx().get<project>();
  l_json["project_config"] = l_prj;
  l_json["user"]           = {};
  co_await boost::asio::post(boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));

  co_return in_handle->make_msg(l_json.dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> model_base_open_pose(session_data_ptr in_handle) {
  if (in_handle->content_type_ != detail::content_type::application_json) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "bad content type");
  }
  FSys::path l_path;
  try {
    l_path = std::get<nlohmann::json>(in_handle->body_)["path"].get<std::string>();
  } catch (const std::exception& e) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, e.what());
  }

  if (!FSys::exists(l_path)) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "传入路径不存在");
  }

  if (FSys::is_directory(l_path)) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "传入路径是目录");
  }

  DOODLE_TO_MAIN_THREAD();
  auto& l_data                = g_ctx().get<model_base_t>();
  l_data.database_info_.path_ = l_path;
  l_data.registry_            = {};
  try {
    database_n::obs_all l_obs{};
    auto l_k_con = g_ctx().emplace<database_info>().get_connection_const();
    l_obs.open(l_data.registry_, l_k_con);
    l_data.any_ = l_obs;
  } catch (...) {
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::not_found, boost::current_exception_diagnostic_information()
    );
  }
  nlohmann::json l_json = get_trees(l_data.registry_);
  DOODLE_TO_SELF();

  co_return in_handle->make_msg(l_json.dump());
}
void model_base_reg(http_route& in_route) {
  g_ctx().emplace<model_base_t>();
  in_route
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/model/asset_tree", get_asset_tree)
      )
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/model/ctx", model_base_ctx_get))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::post, "api/doodle/model", model_base_open_pose))

      ;
  // 无操作
}
}