//
// Created by td_main on 2023/10/19.
//
#pragma once
#include <doodle_core/configure/static_value.h>
#include <doodle_core/core/core_help_impl.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/core_sql.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/file_sys.h>
#include <doodle_core/core/global_function.h>
#include <doodle_core/core/status_info.h>
#include <doodle_core/database_task/details/assets.h>
#include <doodle_core/database_task/details/assets_file.h>
#include <doodle_core/database_task/details/comment.h>
#include <doodle_core/database_task/details/database.h>
#include <doodle_core/database_task/details/entt_handle_ref.h>
#include <doodle_core/database_task/details/episodes.h>
#include <doodle_core/database_task/details/file_one_path.h>
#include <doodle_core/database_task/details/image_icon.h>
#include <doodle_core/database_task/details/importance.h>
#include <doodle_core/database_task/details/load_save_impl.h>
#include <doodle_core/database_task/details/project.h>
#include <doodle_core/database_task/details/project_config.h>
#include <doodle_core/database_task/details/rules.h>
#include <doodle_core/database_task/details/season.h>
#include <doodle_core/database_task/details/shot.h>
#include <doodle_core/database_task/details/tag.h>
#include <doodle_core/database_task/details/time_point_wrap.h>
#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/details/user.h>
#include <doodle_core/database_task/select.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/thread_pool/process_message.h>

#include "boost/asio/any_io_executor.hpp"
#include "boost/core/ignore_unused.hpp"
#include <boost/asio.hpp>

#include "entt/entity/fwd.hpp"
#include "entt/signal/sigh.hpp"
#include "range/v3/action/push_back.hpp"
#include "range/v3/action/remove_if.hpp"
#include "range/v3/algorithm/all_of.hpp"
#include "range/v3/algorithm/for_each.hpp"
#include "range/v3/algorithm/transform.hpp"
#include "range/v3/view/filter.hpp"
#include "range/v3/view/map.hpp"
#include <any>
#include <atomic>
#include <entt/entt.hpp>
#include <filesystem>
#include <fmt/core.h>
#include <range/v3/action/unique.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>
#include <tuple>
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

  bool has_update() const { return !obs_update_.empty() || !obs_create_.empty() || !additional_save_handles_.empty(); }

  void connect(entt::registry& in_registry_ptr) {
    obs_update_.connect(in_registry_ptr, entt::collector.update<type_t>().where<database>());
    obs_create_.connect(in_registry_ptr, entt::collector.group<database, type_t>());
  }

  void disconnect() {
    obs_update_.disconnect();
    obs_create_.disconnect();
  }

  void clear() {
    obs_update_.clear();
    obs_create_.clear();
  }

  void open(
      entt::registry& in_registry_ptr, const sql_connection_ptr& in_conn,
      std::map<std::int64_t, entt::handle>& in_handle
  ) {
    database_n::sql_com<type_t> l_table{};
    if (l_table.has_table(in_conn)) l_table.select(in_conn, in_handle, in_registry_ptr);
  };
  void import_handles(const std::vector<entt::handle>& in_handles) {
    additional_save_handles_ |= ranges::actions::push_back(
        in_handles | ranges::views::filter([](const entt::handle& in_h) { return in_h && in_h.any_of<type_t>(); })
    );
  }
  void save(
      entt::registry& in_registry_ptr, const sql_connection_ptr& in_conn, const std::vector<std::int64_t>& in_handle
  ) {
    std::set<entt::entity> l_create{};

    for (auto&& i : obs_create_) {
      l_create.emplace(i);
    }
    for (auto&& i : obs_update_) {
      l_create.emplace(i);
    }

    auto l_handles = l_create | ranges::views::transform([&](const entt::entity& in_e) -> entt::handle {
                       return {in_registry_ptr, in_e};
                     }) |
                     ranges::to_vector;
    l_handles |=
        ranges::actions::push_back(additional_save_handles_ | ranges::views::filter([](const entt::handle& in_h) {
                                     return in_h && in_h.any_of<type_t>();
                                   }));
    l_handles |= ranges::actions::unique;
    if (l_handles.empty() && in_handle.empty()) return;

    database_n::sql_com<type_t> l_orm{};
    if (!l_orm.has_table(in_conn)) l_orm.create_table(in_conn);
    BOOST_ASSERT(ranges::all_of(l_handles, [&](entt::handle& i) { return i.get<database>().is_install(); }));
    auto [l_updata, l_install] = l_orm.split_update_install(in_conn, l_handles);
    if (!l_updata.empty()) l_orm.update(in_conn, l_updata);
    if (!l_install.empty()) l_orm.insert(in_conn, l_install);
    if (!in_handle.empty()) l_orm.destroy(in_conn, in_handle);
  }
  void save_all(
      entt::registry& in_registry_ptr, const sql_connection_ptr& in_conn, const std::vector<std::int64_t>& in_handle
  ) {
    boost::ignore_unused(in_handle);

    auto l_v       = in_registry_ptr.view<database, type_t>();
    auto l_handles = l_v | ranges::views::transform([&](const entt::entity& in_e) -> entt::handle {
                       return {in_registry_ptr, in_e};
                     }) |
                     ranges::to_vector;
    if (l_handles.empty()) return;

    database_n::sql_com<type_t> l_orm{};
    if (!l_orm.has_table(in_conn)) l_orm.create_table(in_conn);
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

  bool has_update() const { return !destroy_ids_.empty() || !obs_create_.empty() || !additional_save_handles_.empty(); }

  void connect(entt::registry& in_registry_ptr) {
    obs_create_.connect(in_registry_ptr, entt::collector.group<database>());
    conn_  = in_registry_ptr.on_destroy<database>().connect<&impl_obs<database>::on_destroy>(*this);
    conn_2 = in_registry_ptr.on_construct<assets_file>().connect<&entt::registry::get_or_emplace<time_point_wrap>>();
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

  void open(
      entt::registry& in_registry_ptr, const sql_connection_ptr& in_conn,
      std::map<std::int64_t, entt::handle>& in_handle
  ) {
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

  void save(entt::registry& in_registry_ptr, const sql_connection_ptr& in_conn, std::vector<std::int64_t>& in_handle) {
    std::set<entt::handle> l_create{};

    for (auto&& i : obs_create_) {
      if (!in_registry_ptr.get<database>(i).is_install()) l_create.emplace(in_registry_ptr, i);
    }
    auto l_handles = l_create | ranges::views::transform([&](const entt::entity& in_e) -> entt::handle {
                       return {in_registry_ptr, in_e};
                     }) |
                     ranges::to_vector;
    l_handles |=
        ranges::actions::push_back(additional_save_handles_ | ranges::views::filter([](const entt::handle& in_h) {
                                     return in_h && in_h.any_of<database>();
                                   })) |
        ranges::actions::unique;

    in_handle = destroy_ids_;
    if (destroy_ids_.empty() && l_handles.empty()) return;

    database_n::sql_com<database> l_orm{};
    if (!l_orm.has_table(in_conn)) l_orm.create_table(in_conn);

    if (!l_handles.empty()) l_orm.insert(in_conn, l_handles);
    if (!destroy_ids_.empty()) l_orm.destroy(in_conn, destroy_ids_);
  }

  void save_all(
      entt::registry& in_registry_ptr, const sql_connection_ptr& in_conn, std::vector<std::int64_t>& in_handle
  ) {
    auto l_v       = in_registry_ptr.view<database>();
    auto l_handles = l_v | ranges::views::transform([&](const entt::entity& in_e) -> entt::handle {
                       return {in_registry_ptr, in_e};
                     }) |
                     ranges::to_vector;
    in_handle = destroy_ids_;

    if (l_handles.empty()) return;
    database_n::sql_com<database> l_orm{};
    if (!l_orm.has_table(in_conn)) l_orm.create_table(in_conn);
    if (!l_handles.empty()) l_orm.insert(in_conn, l_handles);
  }
};

template <typename type_t>
class impl_ctx {
 public:
  void open(entt::registry& in_registry_ptr, const sql_connection_ptr& in_conn) {
    database_n::sql_ctx<type_t> l_table{};
    if (l_table.has_table(in_conn)) {
      if (in_registry_ptr.ctx().contains<type_t>()) in_registry_ptr.ctx().erase<type_t>();
      l_table.select(in_conn, in_registry_ptr.ctx().emplace<type_t>());
    }
  }
  void save(entt::registry& in_registry_ptr, const sql_connection_ptr& in_conn) {
    if (!in_registry_ptr.ctx().contains<type_t>()) return;

    database_n::sql_ctx<type_t> l_table{};
    if (!l_table.has_table(in_conn)) l_table.create_table(in_conn);
    l_table.insert(in_conn, in_registry_ptr.ctx().get<type_t>());
  }
};

template <typename arg_ctx, typename arg_com>
class obs_main {
  template <template <typename...> typename type_ptr, typename T>
  struct tuple_helper;

  template <template <typename...> typename type_ptr, typename... Ts>
  struct tuple_helper<type_ptr, std::tuple<Ts...>> {
    using type = std::tuple<std::shared_ptr<type_ptr<Ts>>...>;

    static auto make_tuple() { return std::make_tuple(std::make_shared<type_ptr<Ts>>()...); }
  };

  using database_cat_tuble  = decltype(std::tuple_cat(std::declval<std::tuple<database>>(), std::declval<arg_com>()));
  using obs_tuple_type_make = tuple_helper<impl_obs, database_cat_tuble>;
  using ctx_tuple_type_make = tuple_helper<impl_ctx, arg_ctx>;
  using obs_tuple_type      = typename obs_tuple_type_make::type;
  using ctx_tuple_type      = typename ctx_tuple_type_make::type;
  obs_tuple_type obs_data_{};
  ctx_tuple_type ctx_data_{};

 public:
  explicit obs_main() : obs_data_{obs_tuple_type_make::make_tuple()}, ctx_data_{ctx_tuple_type_make::make_tuple()} {}

  bool has_update() const {
    return std::apply([&](auto&&... x) { return ((x->has_update() || ...)); }, obs_data_);
  }

  void open(const registry_ptr& in_registry_ptr, const sql_connection_ptr& in_conn) { open(*in_registry_ptr, in_conn); }
  void open(entt::registry& in_registry, const sql_connection_ptr& in_conn) {
    std::map<std::int64_t, entt::handle> l_map{};
    std::apply([&](auto&&... x) { (x->disconnect(), ...); }, obs_data_);
    std::apply([&](auto&&... x) { (x->clear(), ...); }, obs_data_);
    std::apply([&](auto&&... x) { (x->open(in_registry, in_conn), ...); }, ctx_data_);
    std::apply([&](auto&&... x) { (x->open(in_registry, in_conn, l_map), ...); }, obs_data_);
    std::apply([&](auto&&... x) { (x->connect(in_registry), ...); }, obs_data_);
  }

  void open_ctx(const registry_ptr& in_registry_ptr, const sql_connection_ptr& in_conn) {
    std::apply([&](auto&&... x) { (x->disconnect(), ...); }, obs_data_);
    std::apply([&](auto&&... x) { (x->clear(), ...); }, obs_data_);
    std::apply([&](auto&&... x) { (x->open(*in_registry_ptr, in_conn), ...); }, ctx_data_);
    std::apply([&](auto&&... x) { (x->connect(*in_registry_ptr), ...); }, obs_data_);
  }
  //  void add_ref_project(const registry_ptr& in_registry_ptr, const sql_connection_ptr& in_conn) {
  //    std::map<std::int64_t, entt::handle> l_map{};
  //    std::apply([&](auto&&... x) { (x->disconnect(), ...); }, obs_data_);
  //    std::apply([&](auto&&... x) { (x->open(in_registry_ptr, in_conn, l_map), ...); }, obs_data_);
  //    l_map | ranges::views::values |
  //        ranges::views::for_each([](const entt::handle& in_handle) { in_handle.remove<database>(); });
  //    std::apply([&](auto&&... x) { ((x->connect(in_registry_ptr), ...)); }, obs_data_);
  //  }

  void import_project(const registry_ptr& in_registry_ptr, const sql_connection_ptr& in_conn) {
    std::map<std::int64_t, entt::handle> l_map{};
    std::apply([&](auto&&... x) { (x->disconnect(), ...); }, obs_data_);
    std::apply([&](auto&&... x) { (x->open(*in_registry_ptr, in_conn, l_map), ...); }, obs_data_);
    auto l_hs                 = l_map | ranges::views::values | ranges::to_vector;
    auto l_view_data          = in_registry_ptr->view<database>().each();
    std::set<uuid> l_uuid_set = l_view_data |
                                ranges::views::transform([](const decltype(l_view_data)::value_type& in_e) -> uuid {
                                  return std::get<1>(in_e).uuid();
                                }) |
                                ranges::to<std::set>;
    l_hs |= ranges::actions::remove_if([&](const entt::handle& in_handle) -> bool {
      return l_uuid_set.count(in_handle.get<database>().uuid()) == 1;
    });
    std::apply([&](auto&&... x) { (x->connect(*in_registry_ptr), ...); }, obs_data_);
    std::apply([&](auto&&... x) { ((x->import_handles(l_hs), ...)); }, obs_data_);
  }

  void disconnect() {
    std::apply([&](auto&&... x) { (x->disconnect(), ...); }, obs_data_);
  }
  void clear() {
    std::apply([&](auto&&... x) { (x->clear(), ...); }, obs_data_);
  }
  void connect(const registry_ptr& in_registry_ptr) { connect(*in_registry_ptr); }
  void connect(entt::registry& in_registry_ptr) {
    std::apply([&](auto&&... x) { (x->connect(in_registry_ptr), ...); }, obs_data_);
  }
  void save(const registry_ptr& in_registry_ptr, const sql_connection_ptr& in_conn) {
    std::vector<std::int64_t> l_handles{};
    std::apply([&](auto&&... x) { (x->save(*in_registry_ptr, in_conn), ...); }, ctx_data_);
    std::apply([&](auto&&... x) { (x->save(*in_registry_ptr, in_conn, l_handles), ...); }, obs_data_);
    std::apply([&](auto&&... x) { (x->clear(), ...); }, obs_data_);
  }
  void save_all(const registry_ptr& in_registry_ptr, const sql_connection_ptr& in_conn) {
    std::vector<std::int64_t> l_handles{};
    std::apply([&](auto&&... x) { (x->save(*in_registry_ptr, in_conn), ...); }, ctx_data_);
    std::apply([&](auto&&... x) { (x->save_all(*in_registry_ptr, in_conn, l_handles), ...); }, obs_data_);
    std::apply([&](auto&&... x) { (x->clear(), ...); }, obs_data_);
  }
};
using obs_all = obs_main<
    std::tuple<doodle::project, doodle::project_config::base_config>,
    std::tuple<
        doodle::episodes, doodle::shot, doodle::season, doodle::assets, doodle::assets_file, doodle::time_point_wrap,
        doodle::comment, doodle::image_icon, doodle::importance, doodle::business::rules, doodle::user, doodle::project,
        doodle::project_config::base_config>>;
}  // namespace doodle::database_n