//
// Created by TD on 2022/6/2.
//

#include "sqlite_client.h"

#include "doodle_core_fwd.h"
#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/core_sql.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/database_task/details/assets.h>
#include <doodle_core/database_task/details/assets_file.h>
#include <doodle_core/database_task/details/comment.h>
#include <doodle_core/database_task/details/database.h>
#include <doodle_core/database_task/details/episodes.h>
#include <doodle_core/database_task/details/export_file_info.h>
#include <doodle_core/database_task/details/image_icon.h>
#include <doodle_core/database_task/details/importance.h>
#include <doodle_core/database_task/details/macro.h>
#include <doodle_core/database_task/details/project.h>
#include <doodle_core/database_task/details/project_config.h>
#include <doodle_core/database_task/details/redirection_path_info.h>
#include <doodle_core/database_task/details/rules.h>
#include <doodle_core/database_task/details/season.h>
#include <doodle_core/database_task/details/shot.h>
#include <doodle_core/database_task/details/time_point_info.h>
#include <doodle_core/database_task/details/time_point_wrap.h>
#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/details/user.h>
#include <doodle_core/database_task/details/work_task.h>
#include <doodle_core/gui_template/show_windows.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_task.h>
#include <doodle_core/thread_pool/process_message.h>

#include "boost/core/ignore_unused.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/hana/fwd/transform.hpp"
#include <boost/asio.hpp>
#include <boost/hana.hpp>
#include <boost/hana/ext/std/tuple.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "core/core_help_impl.h"
#include "core/file_sys.h"
#include "entt/entity/fwd.hpp"
#include "entt/signal/sigh.hpp"
#include "metadata/project.h"
#include "range/v3/action/push_back.hpp"
#include "range/v3/algorithm/all_of.hpp"
#include "range/v3/algorithm/any_of.hpp"
#include "range/v3/algorithm/for_each.hpp"
#include "range/v3/view/filter.hpp"
#include "range/v3/view/for_each.hpp"
#include "range/v3/view/map.hpp"
#include <core/core_set.h>
#include <core/status_info.h>
#include <database_task/details/database.h>
#include <database_task/select.h>
#include <filesystem>
#include <fmt/core.h>
#include <metadata/metadata.h>
#include <range/v3/action/unique.hpp>
#include <range/v3/all.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>
#include <utility>

namespace doodle::database_n {

template <typename type_t>
class impl_obs {
  entt::observer obs_update_;
  entt::observer obs_create_;
  std::vector<entt::handle> additional_save_handles_{};

 public:
  explicit impl_obs()                  = default;
  impl_obs(const impl_obs&)            = delete;
  impl_obs(impl_obs&&)                 = delete;
  impl_obs& operator=(const impl_obs&) = delete;
  impl_obs& operator=(impl_obs&&)      = delete;
  ~impl_obs()                          = default;

  void connect(const registry_ptr& in_registry_ptr) {
    obs_update_.connect(*in_registry_ptr, entt::collector.update<type_t>().where<database>());
    obs_create_.connect(*in_registry_ptr, entt::collector.group<database, type_t>());
  }

  void disconnect() {
    obs_update_.disconnect();
    obs_create_.disconnect();
  }

  void clear() {
    obs_update_.clear();
    obs_create_.clear();
  }

  void open(const registry_ptr& in_registry_ptr, conn_ptr& in_conn, std::map<std::int64_t, entt::handle>& in_handle) {
    database_n::sql_com<type_t> l_table{};
    if (l_table.has_table(in_conn)) l_table.select(in_conn, in_handle, in_registry_ptr);
  };
  void import_handles(const std::vector<entt::handle>& in_handles) {
    additional_save_handles_ |= ranges::actions::push_back(
        in_handles | ranges::views::filter([](const entt::handle& in_h) { return in_h && in_h.any_of<type_t>(); })
    );
  }
  void save(const registry_ptr& in_registry_ptr, conn_ptr& in_conn, const std::vector<std::int64_t>& in_handle) {
    database_n::sql_com<type_t> l_orm{};
    l_orm.create_table(in_conn);

    std::set<entt::entity> l_create{};

    for (auto&& i : obs_create_) {
      l_create.emplace(i);
    }
    for (auto&& i : obs_update_) {
      l_create.emplace(i);
    }

    auto l_handles = l_create | ranges::views::transform([&](const entt::entity& in_e) -> entt::handle {
                       return {*in_registry_ptr, in_e};
                     }) |
                     ranges::to_vector;
    l_handles |=
        ranges::actions::push_back(additional_save_handles_ | ranges::views::filter([](const entt::handle& in_h) {
                                     return in_h && in_h.any_of<type_t>();
                                   })) |
        ranges::actions::unique;

    BOOST_ASSERT(ranges::all_of(l_handles, [&](entt::handle& i) { return i.get<database>().is_install(); }));
    auto [l_updata, l_install] = l_orm.split_update_install(in_conn, l_handles);
    if (!l_updata.empty()) l_orm.update(in_conn, l_updata);
    if (!l_install.empty()) l_orm.insert(in_conn, l_install);
    if (!in_handle.empty()) l_orm.destroy(in_conn, in_handle);
  }
  void save_all(const registry_ptr& in_registry_ptr, conn_ptr& in_conn, const std::vector<std::int64_t>& in_handle) {
    boost::ignore_unused(in_handle);

    database_n::sql_com<type_t> l_orm{};
    l_orm.create_table(in_conn);

    auto l_v       = in_registry_ptr->view<database, type_t>();
    auto l_handles = l_v | ranges::views::transform([&](const entt::entity& in_e) -> entt::handle {
                       return {*in_registry_ptr, in_e};
                     }) |
                     ranges::to_vector;

    BOOST_ASSERT(ranges::all_of(l_handles, [&](entt::handle& i) { return i.get<database>().is_install(); }));
    l_orm.insert(in_conn, l_handles);
  }
};

template <>
class impl_obs<database> {
  entt::observer obs_create_;
  std::vector<std::int64_t> destroy_ids_{};
  std::vector<entt::handle> additional_save_handles_{};
  entt::connection conn_{}, conn_2{};
  void on_destroy(entt::registry& in_reg, entt::entity in_entt) {
    if (auto& l_data = in_reg.get<database>(in_entt); l_data.is_install()) destroy_ids_.emplace_back(l_data.get_id());
  }

 public:
  explicit impl_obs()                  = default;
  impl_obs(const impl_obs&)            = delete;
  impl_obs(impl_obs&&)                 = delete;
  impl_obs& operator=(const impl_obs&) = delete;
  impl_obs& operator=(impl_obs&&)      = delete;
  ~impl_obs()                          = default;

  void connect(const registry_ptr& in_registry_ptr) {
    obs_create_.connect(*in_registry_ptr, entt::collector.group<database>());
    conn_  = in_registry_ptr->on_destroy<database>().connect<&impl_obs<database>::on_destroy>(*this);
    conn_2 = in_registry_ptr->on_construct<assets_file>().connect<&entt::registry::get_or_emplace<time_point_wrap>>();
  }

  void disconnect() {
    obs_create_.disconnect();
    conn_.release();
    conn_2.release();
  }

  void clear() {
    obs_create_.clear();
    destroy_ids_.clear();
  }

  void open(const registry_ptr& in_registry_ptr, conn_ptr& in_conn, std::map<std::int64_t, entt::handle>& in_handle) {
    boost::ignore_unused(this);
    database_n::sql_com<database> l_table{};
    if (l_table.has_table(in_conn)) l_table.select(in_conn, in_handle, in_registry_ptr);
  };

  void import_handles(const std::vector<entt::handle>& in_handles) {
    ranges::for_each(in_handles, [](const entt::handle& in_h) {
      if (in_h.any_of<database>()) in_h.get<database>().set_id(0);
    });
    additional_save_handles_ |= ranges::actions::push_back(in_handles);
  }

  void save(const registry_ptr& in_registry_ptr, conn_ptr& in_conn, std::vector<std::int64_t>& in_handle) {
    database_n::sql_com<database> l_orm{};
    if (!l_orm.has_table(in_conn)) l_orm.create_table(in_conn);

    std::set<entt::handle> l_create{};

    for (auto&& i : obs_create_) {
      if (!in_registry_ptr->get<database>(i).is_install()) l_create.emplace(*in_registry_ptr, i);
    }
    auto l_handles = l_create | ranges::views::transform([&](const entt::entity& in_e) -> entt::handle {
                       return {*in_registry_ptr, in_e};
                     }) |
                     ranges::to_vector;
    l_handles |=
        ranges::actions::push_back(additional_save_handles_ | ranges::views::filter([](const entt::handle& in_h) {
                                     return in_h && in_h.any_of<database>();
                                   })) |
        ranges::actions::unique;

    in_handle = destroy_ids_;
    if (!l_handles.empty()) l_orm.insert(in_conn, l_handles);
    if (!destroy_ids_.empty()) l_orm.destroy(in_conn, destroy_ids_);
  }

  void save_all(const registry_ptr& in_registry_ptr, conn_ptr& in_conn, std::vector<std::int64_t>& in_handle) {
    database_n::sql_com<database> l_orm{};
    if (!l_orm.has_table(in_conn)) l_orm.create_table(in_conn);
    auto l_v       = in_registry_ptr->view<database>();
    auto l_handles = l_v | ranges::views::transform([&](const entt::entity& in_e) -> entt::handle {
                       return {*in_registry_ptr, in_e};
                     }) |
                     ranges::to_vector;
    in_handle = destroy_ids_;
    if (!l_handles.empty()) l_orm.insert(in_conn, l_handles);
  }
};

template <typename type_t>
class impl_ctx {
 public:
  void open(const registry_ptr& in_registry_ptr, conn_ptr& in_conn) {
    database_n::sql_ctx<type_t> l_table{};
    if (l_table.has_table(in_conn)) l_table.select(in_conn, in_registry_ptr->ctx().emplace<type_t>());
  }
  void save(const registry_ptr& in_registry_ptr, conn_ptr& in_conn) {
    if (!in_registry_ptr->ctx().contains<type_t>()) return;

    database_n::sql_ctx<type_t> l_table{};
    if (!l_table.has_table(in_conn)) l_table.create_table(in_conn);
    l_table.insert(in_conn, in_registry_ptr->ctx().get<type_t>());
  }
};

template <typename arg_ctx, typename arg_com>
class obs_main {
  template <template <typename> typename type_ptr, typename T>
  struct tuple_helper;

  template <template <typename> typename type_ptr, typename... Ts>
  struct tuple_helper<type_ptr, std::tuple<Ts...>> {
    using type = std::tuple<std::shared_ptr<type_ptr<Ts>>...>;

    static auto make_tuple() { return std::make_tuple(std::make_shared<type_ptr<Ts>>()...); }
  };
  using obs_tuple_type_make = tuple_helper<impl_obs, arg_com>;
  using ctx_tuple_type_make = tuple_helper<impl_ctx, arg_ctx>;
  using obs_tuple_type      = typename obs_tuple_type_make::type;
  using ctx_tuple_type      = typename ctx_tuple_type_make::type;
  obs_tuple_type obs_data_{};
  ctx_tuple_type ctx_data_{};

 public:
  explicit obs_main() : obs_data_{obs_tuple_type_make::make_tuple()}, ctx_data_{ctx_tuple_type_make::make_tuple()} {}

  void open(const registry_ptr& in_registry_ptr, conn_ptr& in_conn) {
    std::map<std::int64_t, entt::handle> l_map{};
    std::apply([&](auto&&... x) { (x->disconnect(), ...); }, obs_data_);
    std::apply([&](auto&&... x) { (x->clear(), ...); }, obs_data_);
    std::apply([&](auto&&... x) { (x->open(in_registry_ptr, in_conn), ...); }, ctx_data_);
    std::apply([&](auto&&... x) { (x->open(in_registry_ptr, in_conn, l_map), ...); }, obs_data_);
    std::apply([&](auto&&... x) { (x->connect(in_registry_ptr), ...); }, obs_data_);
  }

  //  void add_ref_project(const registry_ptr& in_registry_ptr, conn_ptr& in_conn) {
  //    std::map<std::int64_t, entt::handle> l_map{};
  //    std::apply([&](auto&&... x) { (x->disconnect(), ...); }, obs_data_);
  //    std::apply([&](auto&&... x) { (x->open(in_registry_ptr, in_conn, l_map), ...); }, obs_data_);
  //    l_map | ranges::views::values |
  //        ranges::views::for_each([](const entt::handle& in_handle) { in_handle.remove<database>(); });
  //    std::apply([&](auto&&... x) { ((x->connect(in_registry_ptr), ...)); }, obs_data_);
  //  }

  void import_project(const registry_ptr& in_registry_ptr, conn_ptr& in_conn) {
    std::map<std::int64_t, entt::handle> l_map{};
    std::apply([&](auto&&... x) { (x->disconnect(), ...); }, obs_data_);
    std::apply([&](auto&&... x) { (x->open(in_registry_ptr, in_conn, l_map), ...); }, obs_data_);
    std::apply([&](auto&&... x) { (x->connect(in_registry_ptr), ...); }, obs_data_);
    auto l_hs = l_map | ranges::views::values | ranges::to_vector;
    std::apply([&](auto&&... x) { ((x->import_handles(l_hs), ...)); }, obs_data_);
  }

  void disconnect() {
    std::apply([&](auto&&... x) { (x->disconnect(), ...); }, obs_data_);
  }
  void clear() {
    std::apply([&](auto&&... x) { (x->clear(), ...); }, obs_data_);
  }
  void connect(const registry_ptr& in_registry_ptr) {
    std::apply([&](auto&&... x) { (x->connect(in_registry_ptr), ...); }, obs_data_);
  }

  void save(const registry_ptr& in_registry_ptr, conn_ptr& in_conn) {
    std::vector<std::int64_t> l_handles{};
    std::apply([&](auto&&... x) { (x->save(in_registry_ptr, in_conn), ...); }, ctx_data_);
    std::apply([&](auto&&... x) { (x->save(in_registry_ptr, in_conn, l_handles), ...); }, obs_data_);
    std::apply([&](auto&&... x) { (x->clear(), ...); }, obs_data_);
  }
  void save_all(const registry_ptr& in_registry_ptr, conn_ptr& in_conn) {
    std::vector<std::int64_t> l_handles{};
    std::apply([&](auto&&... x) { (x->save_all(in_registry_ptr, in_conn, l_handles), ...); }, obs_data_);
    std::apply([&](auto&&... x) { (x->clear(), ...); }, obs_data_);
  }
};
using obs_all = obs_main<
    std::tuple<doodle::project, doodle::project_config::base_config>,
    std::tuple<
        doodle::episodes, doodle::shot, doodle::season, doodle::assets, doodle::assets_file, doodle::time_point_wrap,
        doodle::comment, doodle::image_icon, doodle::importance, doodle::redirection_path_info, doodle::business::rules,
        doodle::user, doodle::work_task_info>>;

void file_translator::open_begin() {
  is_opening = true;
  doodle_lib::Get().ctx().get<database_info>().path_ =
      project_path.empty() ? FSys::path{database_info::memory_data} : project_path;
  auto& k_msg = g_reg()->ctx().emplace<process_message>();
  k_msg.set_name("加载数据");
  k_msg.set_state(k_msg.run);
  g_reg()->clear();
  g_reg()->ctx().get<core_sig>().project_begin_open(project_path);
}
bsys::error_code file_translator::open() {
  auto l_r = open_impl();
  return l_r;
}

void file_translator::open_end() {
  core_set::get_set().add_recent_project(project_path);
  g_reg()->ctx().get<core_sig>().project_end_open();
  auto& k_msg = g_reg()->ctx().emplace<process_message>();
  k_msg.set_name("完成写入数据");
  k_msg.set_state(k_msg.success);
  g_reg()->ctx().erase<process_message>();
  is_opening = false;
}

void file_translator::save_begin() {
  is_saving   = true;
  auto& k_msg = g_reg()->ctx().emplace<process_message>();
  k_msg.set_name("保存数据");
  k_msg.set_state(k_msg.run);
}

bsys::error_code file_translator::save() {
  auto l_r = save_impl();
  return l_r;
}

void file_translator::save_end() {
  g_reg()->ctx().get<status_info>().need_save = false;
  auto& k_msg                                 = g_reg()->ctx().emplace<process_message>();
  k_msg.set_name("完成写入数据");
  k_msg.set_state(k_msg.success);
  g_reg()->ctx().erase<process_message>();
  is_saving = false;
}

class file_translator::impl {
 public:
  registry_ptr registry_attr;
  std::shared_ptr<obs_all> obs_save;
};

file_translator::file_translator() : ptr(std::make_unique<impl>()) {}
file_translator::file_translator(registry_ptr in_registry) : ptr(std::make_unique<impl>()) {
  ptr->registry_attr = std::move(in_registry);
  ptr->obs_save      = std::make_shared<obs_all>();
}
bsys::error_code file_translator::open_impl() {
  ptr->registry_attr = g_reg();

  bool need_save{};
  {
    database_n::select l_select{};
    auto l_k_con = doodle_lib::Get().ctx().get<database_info>().get_connection_const();
    if (!l_select.is_old(project_path, l_k_con)) {
      ptr->obs_save->open(ptr->registry_attr, l_k_con);
    } else {
      ptr->obs_save->disconnect();
      l_select(*ptr->registry_attr, project_path, l_k_con);
      l_select.patch();
      need_save = true;
      /// 先监听
      ptr->obs_save->connect(ptr->registry_attr);
    }
  }
  for (auto&& [e, p] : ptr->registry_attr->view<project>().each()) {
    ptr->registry_attr->ctx().emplace<project>() = p;
    break;
  }
  for (auto&& [e, p] : ptr->registry_attr->view<project_config::base_config>().each()) {
    ptr->registry_attr->ctx().emplace<project_config::base_config>() = p;
    break;
  }

  if (need_save) {
    if (FSys::folder_is_save(project_path)) {
      project_path.replace_filename(fmt::format("{}_v2.doodle_db", project_path.stem().string()));
      if (!FSys::exists(project_path)) {
        doodle_lib::Get().ctx().get<database_info>().path_ = project_path;
        auto l_k_con = doodle_lib::Get().ctx().get<database_info>().get_connection();
        auto l_tx    = sqlpp::start_transaction(*l_k_con);
        ptr->obs_save->save_all(ptr->registry_attr, l_k_con);
        l_tx.commit();
      } else {
        g_reg()->ctx().get<status_info>().message = fmt::format("{} 位置权限不够, 不保存", project_path);
      }
    }
  }
  ptr->registry_attr->ctx().get<project>().set_path(project_path.parent_path());

  return {};
}
bsys::error_code file_translator::save_impl() {
  if (!FSys::folder_is_save(project_path)) {
    g_reg()->ctx().get<status_info>().message = fmt::format("{} 位置无法写入, 不保存", project_path);
    return bsys::error_code{};
  }

  ptr->registry_attr = g_reg();

  DOODLE_LOG_INFO("文件位置 {}", project_path);
  if (auto l_p = project_path.parent_path(); !FSys::exists(l_p)) {
    FSys::create_directories(l_p);
  }
  doodle_lib::Get().ctx().get<database_info>().path_ = project_path;
  try {
    auto l_k_con = doodle_lib::Get().ctx().get<database_info>().get_connection();
    auto l_tx    = sqlpp::start_transaction(*l_k_con);
    ptr->obs_save->save(ptr->registry_attr, l_k_con);
    l_tx.commit();
  } catch (const sqlpp::exception& in_error) {
    DOODLE_LOG_INFO(boost::diagnostic_information(in_error));
    g_reg()->ctx().get<status_info>().message = "保存失败";
  }

  return {};
}
void file_translator::new_file_scene(const FSys::path& in_path, const project& in_project) {
  ptr->obs_save->disconnect();
  ptr->obs_save->clear();
  doodle_lib::Get().ctx().get<database_info>().path_ = in_path;
  g_reg()->clear();
  ptr->obs_save->connect(ptr->registry_attr);
  entt::handle l_prj{*ptr->registry_attr, ptr->registry_attr->create()};
  entt::handle l_prj_config{*ptr->registry_attr, ptr->registry_attr->create()};
  l_prj.emplace<database>();
  l_prj_config.emplace<database>();
  l_prj.emplace<project>(in_project);
  auto& l_config              = l_prj_config.emplace<project_config::base_config>();
  l_config.maya_camera_select = {
      std::make_pair(R"(front|persp|side|top|camera)"s, -1000),
      std::make_pair(R"(ep\d+_sc\d+)"s, 30),
      std::make_pair(R"(ep\d+)"s, 10),
      std::make_pair(R"(sc\d+)"s, 10),
      std::make_pair(R"(ep_\d+_sc_\d+)"s, 10),
      std::make_pair(R"(ep_\d+)"s, 5),
      std::make_pair(R"(sc_\d+)"s, 5),
      std::make_pair(R"(^[A-Z]+_)"s, 2),
      std::make_pair(R"(_\d+_\d+)"s, 2)};
  l_config.icon_extensions                              = {".png", ".jpg", ".jpeg", ".tga", ".tif"};
  l_config.export_group                                 = "UE4";
  l_config.cloth_proxy_                                 = "_cloth_proxy";
  l_config.simple_module_proxy_                         = "_proxy";
  l_config.maya_camera_suffix                           = "camera";
  l_config.maya_out_put_abc_suffix                      = "_output_abc";
  l_config.season_count                                 = 20;

  g_reg()->ctx().emplace<project>()                     = in_project;
  g_reg()->ctx().emplace<project_config::base_config>() = l_config;

  auto& l_s                                             = ptr->registry_attr->ctx().emplace<status_info>();
  l_s.message                                           = "创建新项目";
  l_s.need_save                                         = true;
}
file_translator::~file_translator() = default;
}  // namespace doodle::database_n
