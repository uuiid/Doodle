//
// Created by TD on 25-8-21.
//
#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/person.h>

#include "doodle_lib/core/http/http_function.h"
#include <doodle_lib/core/entity_path.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/http_method/kitsu/kitsu_result.h>
#include <doodle_lib/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>
#include <doodle_lib/sqlite_orm/sqlite_select_data.h>

#include <any>
#include <map>
#include <memory>
#include <range/v3/range/conversion.hpp>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace doodle::http {
struct actions_projects_casting_replace_arg {
  uuid entity_id_;
  uuid asset_from_id_;
  uuid asset_to_id_;
  // from_json
  friend void from_json(const nlohmann::json& in_json, actions_projects_casting_replace_arg& in_arg) {
    in_json.at("entity_id").get_to(in_arg.entity_id_);
    in_json.at("asset_from_id").get_to(in_arg.asset_from_id_);
    in_json.at("asset_to_id").get_to(in_arg.asset_to_id_);
    if (in_arg.asset_from_id_.is_nil() || in_arg.asset_to_id_.is_nil() || in_arg.entity_id_.is_nil())
      throw_exception(doodle_error{boost::beast::http::status::bad_request, "ID 不能为空"});
  }
};
}  // namespace doodle::http

template <>
struct fmt::formatter<doodle::http::actions_projects_casting_replace_arg> : fmt::formatter<fmt::string_view> {
  template <typename FmtContext>
  auto format(const doodle::http::actions_projects_casting_replace_arg& c, FmtContext& ctx) const {
    return fmt::format_to(ctx.out(), "({}, {}, {})", c.entity_id_, c.asset_from_id_, c.asset_to_id_);
  }
};

namespace doodle::http {
namespace {
struct data_project_sequences_casting_result {
  explicit data_project_sequences_casting_result(
      const entity_link& in_ent_link, const std::string& in_entity_name, const uuid& in_entity_preview_file_id,
      const uuid& in_entity_project_id, const std::string& in_asset_type_name
  )
      : asset_id_(in_ent_link.entity_out_id_),
        name_(in_entity_name),
        asset_name_(in_entity_name),
        asset_type_name_(in_asset_type_name),
        sequence_name_(),
        preview_file_id_(in_entity_preview_file_id),
        nb_occurences_(in_ent_link.nb_occurences_),
        label_(in_ent_link.label_),
        is_shared_(),
        project_id_(in_entity_project_id)

  {}
  uuid asset_id_;
  std::string name_;
  std::string asset_name_;
  std::string asset_type_name_;
  std::string sequence_name_;
  uuid preview_file_id_;
  std::int32_t nb_occurences_;
  std::string label_;
  bool is_shared_;
  uuid project_id_;

  // to json
  friend void to_json(nlohmann::json& j, const data_project_sequences_casting_result& in_data) {
    j["asset_id"]        = in_data.asset_id_;
    j["name"]            = in_data.name_;
    j["asset_name"]      = in_data.asset_name_;
    j["asset_type_name"] = in_data.asset_type_name_;
    j["sequence_name"]   = in_data.sequence_name_;
    j["preview_file_id"] = in_data.preview_file_id_;
    j["nb_occurences"]   = in_data.nb_occurences_;
    j["label"]           = in_data.label_;
    j["is_shared"]       = in_data.is_shared_;
    j["project_id"]      = in_data.project_id_;
  }
};

struct data_project_sequences_casting_result_map {
  std::map<uuid, std::vector<data_project_sequences_casting_result>> maps{};
  // to json
  friend void to_json(nlohmann::json& j, const data_project_sequences_casting_result_map& in_data) {
    for (const auto& [key, value] : in_data.maps) {
      j[fmt::to_string(key)] = value;
    }
  }
};

data_project_sequences_casting_result_map get_sequence_casting(
    const uuid& in_project_id, const person& in_person, const uuid& in_sequence_id = {},
    const std::vector<uuid>& in_shot_ids = {}
) {
  auto l_sql = get_sqlite_database();
  data_project_sequences_casting_result_map l_result{};
  using namespace sqlite_orm;
  constexpr auto shot     = "shot"_alias.for_<entity>();
  constexpr auto sequence = "sequence"_alias.for_<entity>();
  auto l_outsource_select = select(
      &outsource_studio_authorization::entity_id_,
      where(c(&outsource_studio_authorization::studio_id_) == in_person.studio_id_)
  );

  for (auto&& [ent_link, entity_name_, entity_preview_file_id_, entity_project_id_, asset_type_name_] :
       l_sql.impl_->storage_any_.select(
           columns(
               object<entity_link>(true), &entity::name_, &entity::preview_file_id_, &entity::project_id_,
               &asset_type::name_
           ),
           from<entity_link>(), join<shot>(on(c(&entity_link::entity_in_id_) == c(shot->*&entity::uuid_id_))),
           join<sequence>(on(c(shot->*&entity::parent_id_) == c(sequence->*&entity::uuid_id_))),
           join<entity>(on(c(&entity_link::entity_out_id_) == c(&entity::uuid_id_))),
           join<asset_type>(on(c(&entity::entity_type_id_) == c(&asset_type::uuid_id_))),
           where(
               c(&entity::canceled_) != true && (in_project_id.is_nil() || c(&entity::project_id_) == in_project_id) &&
               (in_sequence_id.is_nil() || c(sequence->*&entity::uuid_id_) == in_sequence_id) &&
               (in_shot_ids.empty() || in(shot->*&entity::uuid_id_, in_shot_ids)) &&  //
               (in_person.role_ != person_role_type::outsource ||
                (                                                 //
                    in(&entity::uuid_id_, l_outsource_select) ||  //
                    in(sequence->*&entity::uuid_id_, l_outsource_select)
                ))
           ),
           multi_order_by(
               order_by(sequence->*&entity::name_), order_by(shot->*&entity::name_), order_by(&asset_type::name_),
               order_by(&entity::name_)
           )
       )) {
    l_result.maps[ent_link.entity_in_id_].emplace_back(
        data_project_sequences_casting_result{
            ent_link, entity_name_, entity_preview_file_id_, entity_project_id_, asset_type_name_
        }
    );
  }
  return l_result;
}

struct data_project_asset_types_casting_result {
  explicit data_project_asset_types_casting_result(
      const entity_link& in_ent_link, const std::string& in_name, const std::string& in_asset_name,
      const std::string& in_asset_type_name, const uuid& in_preview_file_id
  )
      : asset_id_(in_ent_link.entity_out_id_),
        name_(in_name),
        asset_name_(in_asset_name),
        asset_type_name_(in_asset_type_name),
        preview_file_id_(in_preview_file_id),
        nb_occurences_(in_ent_link.nb_occurences_),
        label_(in_ent_link.label_) {}
  uuid asset_id_;
  std::string name_;
  std::string asset_name_;
  std::string asset_type_name_;
  uuid preview_file_id_;
  std::int32_t nb_occurences_;
  std::string label_;
  // to json
  friend void to_json(nlohmann::json& j, const data_project_asset_types_casting_result& in_data) {
    j["asset_id"]        = in_data.asset_id_;
    j["name"]            = in_data.name_;
    j["asset_name"]      = in_data.asset_name_;
    j["asset_type_name"] = in_data.asset_type_name_;
    j["preview_file_id"] = in_data.preview_file_id_;
    j["nb_occurences"]   = in_data.nb_occurences_;
    j["label"]           = in_data.label_;
  }
};

struct data_project_asset_types_casting_result_map {
  std::map<uuid, std::vector<data_project_asset_types_casting_result>> maps{};
  // to json
  friend void to_json(nlohmann::json& j, const data_project_asset_types_casting_result_map& in_data) {
    for (const auto& [key, value] : in_data.maps) {
      j[fmt::to_string(key)] = value;
    }
  }
};

data_project_asset_types_casting_result_map get_asset_type_casting(
    const uuid& in_project_id, const uuid& in_asset_type_id
) {
  auto l_sql = get_sqlite_database();
  data_project_asset_types_casting_result_map l_result{};
  using namespace sqlite_orm;
  constexpr auto asset = "asset"_alias.for_<entity>();
  for (auto&& [ent_link, entity_name_, entity_preview_file_id_, entity_project_id_, asset_type_name_] :
       l_sql.impl_->storage_any_.select(

           columns(
               object<entity_link>(true), &entity::name_, &entity::preview_file_id_, &entity::project_id_,
               &asset_type::name_
           ),
           from<entity_link>(),  //
           join<asset>(on(c(&entity_link::entity_in_id_) == c(asset->*&entity::uuid_id_))),
           join<entity>(on(c(&entity_link::entity_out_id_) == c(&entity::uuid_id_))),
           join<asset_type>(on(c(&entity::entity_type_id_) == c(&asset_type::uuid_id_))),
           where(
               c(&entity::canceled_) != true && (in_project_id.is_nil() || c(&entity::project_id_) == in_project_id) &&
               (in_asset_type_id.is_nil() || c(&entity::entity_type_id_) == in_asset_type_id)
           ),
           multi_order_by(order_by(&asset_type::name_), order_by(&entity::name_))

       )) {
    l_result.maps[ent_link.entity_in_id_].emplace_back(
        data_project_asset_types_casting_result{
            ent_link, entity_name_, entity_name_, asset_type_name_, entity_preview_file_id_
        }
    );
  }
  return l_result;
}

}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> data_project_sequences_all_casting::get(
    session_data_ptr in_handle
) {
  person_.check_in_project(project_id_);
  co_return in_handle->make_msg(nlohmann::json{} = get_sequence_casting(project_id_, person_.person_));
}

namespace {
auto get_entity_link_by_entity_id(const uuid& in_entity_id) {
  auto l_sql = get_sqlite_database();

  using namespace sqlite_orm;
  auto l_ret = l_sql.impl_->storage_any_.get_all<entity_link>(where(c(&entity_link::entity_in_id_) == in_entity_id));

  return l_ret;
}

auto get_entity_link_by_entity_id(const std::vector<uuid>& in_entity_id) {
  auto l_sql = get_sqlite_database();

  using namespace sqlite_orm;
  auto l_ret = l_sql.impl_->storage_any_.get_all<entity_link>(where(in(&entity_link::entity_in_id_, in_entity_id)));

  return l_ret;
}

std::optional<entity_link> find_entity_link(std::vector<entity_link>& in_entity_links, const uuid& in_entity_out_id) {
  auto l_it =
      std::find_if(in_entity_links.begin(), in_entity_links.end(), [&in_entity_out_id](const entity_link& in_link) {
        return in_link.entity_out_id_ == in_entity_out_id;
      });

  if (l_it != in_entity_links.end()) {
    auto l_ret = *l_it;
    in_entity_links.erase(l_it);
    return l_ret;
  }
  return std::nullopt;
}

struct get_casting_t {
 public:
  uuid asset_id_;
  std::string asset_name_;
  std::string asset_type_name_;
  uuid ready_for_;
  uuid episode_id_;
  uuid preview_file_id_;
  std::int32_t nb_occurences_;
  std::string label_;
  bool is_shared_;
  uuid project_id_;

  static std::vector<get_casting_t> get(const uuid& in_entity_id) {
    auto l_sql = get_sqlite_database();
    using namespace sqlite_orm;
    auto l_r = l_sql.impl_->storage_any_.select(
        columns(
            object<entity_link>(true), &entity::name_, &asset_type::name_, &entity::ready_for_, &entity::source_id_,
            &entity::preview_file_id_, &entity::project_id_
        ),
        from<entity_link>(), join<entity>(on(c(&entity_link::entity_out_id_) == c(&entity::uuid_id_))),
        join<asset_type>(on(c(&entity::entity_type_id_) == c(&asset_type::uuid_id_))),
        where(c(&entity_link::entity_in_id_) == in_entity_id && c(&entity::canceled_) != true),
        multi_order_by(order_by(&asset_type::name_), order_by(&entity::name_))
    );
    std::vector<get_casting_t> l_ret{};
    for (auto&& [ent_link, entity_name_, asset_type_name_, ready_for_, episode_id_, preview_file_id_, project_id_] :
         l_r) {
      l_ret.emplace_back(
          get_casting_t{
              ent_link.entity_out_id_, entity_name_, asset_type_name_, ready_for_, episode_id_, preview_file_id_,
              ent_link.nb_occurences_, ent_link.label_, false, project_id_
          }
      );
    }
    return l_ret;
  }
  // to json
  friend void to_json(nlohmann::json& j, const get_casting_t& in_data) {
    j["asset_id"]        = in_data.asset_id_;
    j["asset_name"]      = in_data.asset_name_;
    j["asset_type_name"] = in_data.asset_type_name_;
    j["ready_for"]       = in_data.ready_for_;
    j["episode_id"]      = in_data.episode_id_;
    j["preview_file_id"] = in_data.preview_file_id_;
    j["nb_occurences"]   = in_data.nb_occurences_;
    j["label"]           = in_data.label_;
    j["is_shared"]       = in_data.is_shared_;
    j["project_id"]      = in_data.project_id_;
  }
};

}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_project_entities_casting, get) {
  person_.check_in_project(project_id_);
  person_.check_not_outsourcer();
  if (get_sqlite_database().uuid_to_id<entity>(entity_id_) == 0) throw_exception(doodle_error{"实体不存在或已被删除"});
  co_return in_handle->make_msg(nlohmann::json{} = get_casting_t::get(entity_id_));
}

boost::asio::awaitable<boost::beast::http::message_generator> data_project_entities_casting::put(
    session_data_ptr in_handle
) {
  person_.check_in_project(project_id_);
  auto l_sql = get_sqlite_database();
  auto l_ent = std::make_shared<entity>(l_sql.get_by_uuid<entity>(entity_id_));
  if (l_ent->entity_type_id_ == l_sql.get_entity_type_by_name(std::string{doodle_config::entity_type_episode}).uuid_id_)
    throw_exception(doodle_error{"不能将 Episode 作为实体类型进行操作"});

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始更新 Casting 关联 project_id {} entity_id {}",
      person_.person_.email_, person_.person_.get_full_name(), project_id_, l_ent->uuid_id_
  );
  auto l_entity_links_update = std::make_shared<std::vector<entity_link>>();
  auto l_entity_links_insert = std::make_shared<std::vector<entity_link>>();
  std::vector<std::function<void()>> l_delay_events{};
  auto l_seq        = l_sql.get_by_uuid<entity>(l_ent->parent_id_);

  auto l_shot_links = get_entity_link_by_entity_id(entity_id_);
  auto l_seq_links  = get_entity_link_by_entity_id(l_seq.uuid_id_);

  auto l_list       = in_handle->get_json().get<std::vector<entity_link>>();
  for (auto&& i : l_list) {
    if (i.entity_out_id_.is_nil()) continue;
    auto l_link = find_entity_link(l_shot_links, i.entity_out_id_);
    if (l_link) {
      l_link->nb_occurences_ = i.nb_occurences_;
      l_link->label_         = i.label_;
      l_entity_links_update->emplace_back(*l_link);
      l_delay_events.emplace_back([i, this]() {
        socket_io::broadcast(
            socket_io::entity_link_update_broadcast_t{
                .entity_link_id_ = i.uuid_id_, .nb_occurences_ = i.nb_occurences_, .project_id_ = project_id_
            }
        );
      });
    } else {
      auto l_seq_link = find_entity_link(l_seq_links, i.entity_out_id_);
      if (!l_seq_link) {
        l_entity_links_insert->emplace_back(
            entity_link{
                .entity_in_id_  = l_seq.uuid_id_,
                .entity_out_id_ = i.entity_out_id_,
                .nb_occurences_ = 1,
                .label_         = i.label_
            }
        );
        l_delay_events.emplace_back([id = i.entity_out_id_, this]() {
          socket_io::broadcast(socket_io::asset_update_broadcast_t{.asset_id_ = id, .project_id_ = project_id_});
        });
      }
      l_entity_links_insert->emplace_back(
          entity_link{
              .entity_in_id_  = l_ent->uuid_id_,
              .entity_out_id_ = i.entity_out_id_,
              .nb_occurences_ = i.nb_occurences_,
              .label_         = i.label_
          }
      );
      l_delay_events.emplace_back([i, this]() {
        socket_io::broadcast(
            socket_io::entity_link_new_broadcast_t{
                .entity_link_id_ = i.uuid_id_,
                .entity_in_id_   = i.entity_in_id_,
                .entity_out_id_  = i.entity_out_id_,
                .nb_occurences_  = i.nb_occurences_,
                .project_id_     = project_id_
            }
        );
      });
    }
  }
  if (auto l_id_list = l_shot_links | ranges::views::transform(&entity_link::id_) | ranges::to_vector;
      !l_id_list.empty()) {
    co_await l_sql.remove<entity_link>(l_id_list);
    SPDLOG_LOGGER_WARN(
        g_logger_ctrl().get_http(), "用户 {}({}) 移除实体链接 count {}", person_.person_.email_,
        person_.person_.get_full_name(), l_id_list.size()
    );
  }

  l_ent->nb_entities_out_ = l_list.size();
  co_await l_sql.update(l_ent);
  if (!l_entity_links_update->empty()) {
    co_await l_sql.update_range(l_entity_links_update);
    SPDLOG_LOGGER_WARN(
        g_logger_ctrl().get_http(), "用户 {}({}) 更新实体链接 count {}", person_.person_.email_,
        person_.person_.get_full_name(), l_entity_links_update->size()
    );
  }
  if (!l_entity_links_insert->empty()) {
    co_await l_sql.install_range(l_entity_links_insert);
    SPDLOG_LOGGER_WARN(
        g_logger_ctrl().get_http(), "用户 {}({}) 新增实体链接 count {}", person_.person_.email_,
        person_.person_.get_full_name(), l_entity_links_insert->size()
    );
  }
  // for (auto&& i : l_delay_events) i();
  socket_io::broadcast(
      socket_io::shot_casting_update_broadcast_t{
          .shot_id_ = l_ent->uuid_id_, .project_id_ = project_id_, .nb_entities_out_ = l_ent->nb_entities_out_
      }
  );

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(),
      "用户 {}({}) 完成更新 Casting 关联 project_id {} entity_id {} nb_entities_out {} updated_link_count {} "
      "inserted_link_count {}",
      person_.person_.email_, person_.person_.get_full_name(), project_id_, l_ent->uuid_id_, l_ent->nb_entities_out_,
      l_entity_links_update->size(), l_entity_links_insert->size()
  );
  co_return in_handle->make_msg(in_handle->get_json());
}
boost::asio::awaitable<boost::beast::http::message_generator> data_project_sequences_casting::get(
    session_data_ptr in_handle
) {
  person_.check_in_project(project_id_);
  person_.check_not_outsourcer();
  if (get_sqlite_database().uuid_to_id<entity>(sequence_id_) == 0)
    throw_exception(doodle_error{"序列不存在或已被删除"});
  co_return in_handle->make_msg(nlohmann::json{} = get_sequence_casting(project_id_, person_.person_, sequence_id_));
}
boost::asio::awaitable<boost::beast::http::message_generator> data_project_asset_types_casting::get(
    session_data_ptr in_handle
) {
  person_.check_in_project(project_id_);
  person_.check_not_outsourcer();
  if (get_sqlite_database().uuid_to_id<asset_type>(asset_type_id_) == 0)
    throw_exception(doodle_error{"序列不存在或已被删除"});
  co_return in_handle->make_msg(nlohmann::json{} = get_asset_type_casting(project_id_, asset_type_id_));
}

boost::asio::awaitable<boost::beast::http::message_generator> actions_projects_casting_replace::post(
    session_data_ptr in_handle
) {
  person_.check_in_project(project_id_);
  person_.check_not_outsourcer();
  auto l_list = in_handle->get_json().get<std::vector<actions_projects_casting_replace_arg>>();
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始替换 Casting project_id {} item_count {}", person_.person_.email_,
      person_.person_.get_full_name(), project_id_, l_list.size()
  );
  auto l_sql                                               = get_sqlite_database();
  std::shared_ptr<std::vector<entity_link>> l_entity_links = std::make_shared<std::vector<entity_link>>();
  std::vector<std::function<void()>> l_delay_events{};
  auto l_shot_linke = get_entity_link_by_entity_id(
      l_list | ranges::views::transform(&actions_projects_casting_replace_arg::entity_id_) | ranges::to_vector
  );
  for (auto&& i : l_list) {
    auto l_link = find_entity_link(l_shot_linke, i.asset_from_id_);
    if (!l_link) continue;
    l_link->entity_out_id_ = i.asset_to_id_;
    l_entity_links->emplace_back(*l_link);
    l_delay_events.emplace_back([&, this]() {
      socket_io::broadcast(
          socket_io::entity_link_update_broadcast_t{
              .entity_link_id_ = l_link->uuid_id_, .nb_occurences_ = l_link->nb_occurences_, .project_id_ = project_id_
          }
      );
    });
  }
  co_await l_sql.update_range(l_entity_links);
  for (auto&& i : l_delay_events) i();
  data_project_sequences_casting_result_map l_result{};
  std::vector<uuid> l_shot_ids{};
  for (auto&& i : l_list) l_shot_ids.push_back(i.entity_id_);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成替换 Casting project_id {} item_count {} updated_link_count {}",
      person_.person_.email_, person_.person_.get_full_name(), project_id_, l_list.size(), l_entity_links->size()
  );

  co_return in_handle->make_msg(nlohmann::json{} = get_sequence_casting({}, person_.person_, {}, l_shot_ids));
}
struct actions_projects_casting_copy_arg {
  uuid source_sequence_id_;
  uuid target_sequence_id_;
  // from_json
  friend void from_json(const nlohmann::json& in_json, actions_projects_casting_copy_arg& in_arg) {
    in_json.at("source_sequence_id").get_to(in_arg.source_sequence_id_);
    in_json.at("target_sequence_id").get_to(in_arg.target_sequence_id_);
    if (in_arg.source_sequence_id_.is_nil() || in_arg.target_sequence_id_.is_nil())
      throw_exception(doodle_error{boost::beast::http::status::bad_request, "ID 不能为空"});
  }
};

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_projects_casting_copy, post) {
  auto l_arg = in_handle->get_json().get<actions_projects_casting_copy_arg>();
  person_.check_in_project(project_id_);
  person_.check_not_outsourcer();
  auto l_sql                  = get_sqlite_database();

  auto l_install_entity_links = std::make_shared<std::vector<entity_link>>();
  {
    using namespace sqlite_orm;
    constexpr auto shot     = "shot"_alias.for_<entity>();
    constexpr auto sequence = "sequence"_alias.for_<entity>();
    auto l_source_casting   = l_sql.impl_->storage_any_.get_all<entity_link>(
        join<shot>(on(c(&entity_link::entity_in_id_) == c(shot->*&entity::uuid_id_))),
        join<sequence>(on(c(shot->*&entity::parent_id_) == c(sequence->*&entity::uuid_id_))),
        where(c(sequence->*&entity::uuid_id_) == l_arg.source_sequence_id_)
    );
    // 查找 shot
    auto l_source_shots = l_sql.impl_->storage_any_.get_all<entity>(
        where(c(&entity::parent_id_) == l_arg.source_sequence_id_ && c(&entity::canceled_) != true)
    );
    auto l_target_shots = l_sql.impl_->storage_any_.get_all<entity>(
        where(c(&entity::parent_id_) == l_arg.target_sequence_id_ && c(&entity::canceled_) != true)
    );
    std::map<uuid, uuid> l_shot_id_map{};
    std::map<std::string, entity*> l_target_shot_name_map{};
    for (auto&& target_shot : l_target_shots) l_target_shot_name_map[target_shot.name_] = &target_shot;
    // 按照名称匹配 shot，生成 shot_id_map
    for (auto&& source_shot : l_source_shots)
      if (l_target_shot_name_map.contains(source_shot.name_))
        l_shot_id_map[source_shot.uuid_id_] = l_target_shot_name_map[source_shot.name_]->uuid_id_;

    for (auto&& i : l_source_casting) {
      if (!l_shot_id_map.contains(i.entity_in_id_)) continue;
      l_install_entity_links->emplace_back(
          entity_link{
              .entity_in_id_  = l_shot_id_map[i.entity_in_id_],
              .entity_out_id_ = i.entity_out_id_,
              .nb_occurences_ = i.nb_occurences_,
              .label_         = i.label_
          }
      );
    }
  }
  if (!l_install_entity_links->empty()) {
    SPDLOG_LOGGER_WARN(
        g_logger_ctrl().get_http(),
        "用户 {}({}) 开始复制 Casting project_id {} source_sequence_id {} target_sequence_id "
        "{} copied_link_count {}",
        person_.person_.email_, person_.person_.get_full_name(), project_id_, l_arg.source_sequence_id_,
        l_arg.target_sequence_id_, l_install_entity_links->size()
    );
    co_await l_sql.remove_sequence_casting(l_arg.target_sequence_id_);
    co_await l_sql.install_range(l_install_entity_links);
  }
  co_return in_handle->make_msg(
      nlohmann::json{} = get_sequence_casting(project_id_, person_.person_, l_arg.target_sequence_id_)
  );
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_projects_sequences_casting_ue_assembly_harvest, post) {
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();
  auto l_seq = l_sql.get_by_uuid<entity>(id_);
  auto l_prj = l_sql.get_by_uuid<project>(project_id_);
  episodes l_episodes{l_seq};
  struct shot_harvest_t {
    entity shot_entity_;
    entity_shot_extend shot_extend_;
    entity* sequence_entity_;
    episodes* episode_;
    project* prj_;
    std::vector<std::tuple<entity_asset_extend, uuid>>* seq_entts_;
    std::map<std::string, std::size_t>* seq_entts_all_map_;
    std::vector<std::tuple<uuid, entity_asset_extend, uuid>> shot_entts_;

    std::vector<std::string> assembly_names_{};
    std::vector<std::string> assembly_names_character_{};
    std::vector<std::string> assembly_names_prop_{};
    void operator()(
        std::shared_ptr<std::vector<entity_link>>& out_entity_links, std::shared_ptr<std::vector<entity>>& out_entities
    ) {
      DOODLE_CHICK_HTTP(
          shot_extend_.frame_in_, bad_request, "镜头 {} 实体扩展信息缺少帧起始，请联系管理员添加扩展信息",
          shot_entity_.name_
      );
      DOODLE_CHICK_HTTP(
          shot_extend_.frame_out_, bad_request, "镜头 {} 实体扩展信息缺少帧结束，请联系管理员添加扩展信息",
          shot_entity_.name_
      );

      FSys::path l_path_dir = get_shots_animation_output_path(sequence_entity_->name_, shot_entity_.name_, prj_->code_);
      l_path_dir            = prj_->path_ / l_path_dir;
      if (!FSys::exists(l_path_dir)) return;
      auto l_file_end_str = fmt::format("_{}-{}", shot_extend_.frame_in_.value(), shot_extend_.frame_out_.value());
      auto l_shot_file_name =
          get_shots_animation_file_name(sequence_entity_->name_, shot_entity_.name_, prj_->code_).generic_string();
      l_shot_file_name += '_';

      for (auto&& l_file : FSys::directory_iterator(l_path_dir)) {
        auto l_stem = l_file.path().stem().string();
        if (!(
                l_stem.ends_with(l_file_end_str) && l_stem.starts_with(l_shot_file_name) && l_file.is_regular_file() &&
                l_file.path().extension() == ".fbx"

            ))
          continue;
        auto l_assembly_name =
            l_stem.substr(l_shot_file_name.size(), l_stem.size() - l_shot_file_name.size() - l_file_end_str.size());
        if (l_assembly_name == "camera") continue;
        // 以 Low + 数字 + _ 结尾的组件，去除 Low + 数字 + _ 后缀
        const static std::regex low_regex(R"((_Low\d*)$)");
        l_assembly_name = std::regex_replace(l_assembly_name, low_regex, "");
        const static std::regex rig_regex(R"((_rig_?[a-zA-Z]*\d*)$)");
        l_assembly_name   = std::regex_replace(l_assembly_name, rig_regex, "");
        // 去除开头的 Ch
        bool is_character = l_assembly_name.starts_with("Ch");
        if (is_character) l_assembly_name = l_assembly_name.substr(2);

        if (l_assembly_name.empty()) continue;
        assembly_names_.emplace_back(l_assembly_name);
        if (is_character)
          assembly_names_character_.emplace_back(l_assembly_name);
        else
          assembly_names_prop_.emplace_back(l_assembly_name);
      }
      SPDLOG_INFO("Harvested {} assemblies for shot {}", fmt::join(assembly_names_, ", "), shot_entity_.name_);

      if (assembly_names_.empty()) return;

      decltype(std::decay_t<decltype(*seq_entts_)>()) l_ass{};
      // 查找对应的资产
      for (auto&& l_ass_name : assembly_names_character_) {
        if (!seq_entts_all_map_->contains(l_ass_name)) continue;
        if (std::get<1>(seq_entts_->at(seq_entts_all_map_->at(l_ass_name))) == asset_type::get_character_id())
          l_ass.push_back(seq_entts_->at(seq_entts_all_map_->at(l_ass_name)));
      }
      for (auto&& l_ass_name : assembly_names_prop_) {
        if (!seq_entts_all_map_->contains(l_ass_name)) continue;
        if (std::get<1>(seq_entts_->at(seq_entts_all_map_->at(l_ass_name))) == asset_type::get_prop_id())
          l_ass.push_back(seq_entts_->at(seq_entts_all_map_->at(l_ass_name)));
      }

      std::set<std::string> l_key_names{};
      std::set<uuid> l_key_uuids{};
      // 将已有的资产进行key提取, 在后面去重使用
      for (auto&& [l_uuid, l_ass_ext, l_entity_type_id] : shot_entts_) {
        l_key_uuids.insert(l_uuid);
        if (!l_ass_ext.pin_yin_ming_cheng_.empty() && l_entity_type_id == asset_type::get_prop_id())
          l_key_names.insert(
              l_ass_ext.ban_ben_.empty() ? l_ass_ext.pin_yin_ming_cheng_
                                         : fmt::format("{}_{}", l_ass_ext.pin_yin_ming_cheng_, l_ass_ext.ban_ben_)
          );
        if (!l_ass_ext.bian_hao_.empty() && l_entity_type_id == asset_type::get_character_id())
          l_key_names.insert(l_ass_ext.bian_hao_);
      }

      auto l_find = [&](const entity_asset_extend& in_ext, bool is_prop) {
        if (l_key_uuids.contains(in_ext.entity_id_)) return true;
        auto l_name = is_prop ? in_ext.ban_ben_.empty()
                                    ? in_ext.pin_yin_ming_cheng_
                                    : fmt::format("{}_{}", in_ext.pin_yin_ming_cheng_, in_ext.ban_ben_)
                              : in_ext.bian_hao_;

        return l_key_names.contains(l_name);
      };
      for (auto&& [l_ass_ext, l_entity_type_id] : l_ass) {
        // 重复就不加入
        if (l_find(l_ass_ext, l_entity_type_id == asset_type::get_prop_id())) continue;
        out_entity_links->emplace_back(
            entity_link{
                .entity_in_id_  = shot_entity_.uuid_id_,
                .entity_out_id_ = l_ass_ext.entity_id_,
                .nb_occurences_ = 1,
                .label_         = "animate"
            }
        );
        ++shot_entity_.nb_entities_out_;
      }
      if (!l_ass.empty()) out_entities->emplace_back(shot_entity_);
    }
  };

  using namespace sqlite_orm;
  constexpr auto shot     = "shot"_alias.for_<entity>();
  constexpr auto sequence = "sequence"_alias.for_<entity>();
  // 直接获取当集资产, 后续不使用数据库进行匹配, 人工匹配
  auto l_ass_all          = l_sql.impl_->storage_any_.select(
      columns(object<entity_asset_extend>(), &entity::entity_type_id_), from<entity_asset_extend>(),
      join<entity>(on(c(&entity::uuid_id_) == c(&entity_asset_extend::entity_id_))),
      where(
          c(&entity::project_id_) == project_id_ &&
          in(&entity_asset_extend::entity_id_,
                      select(
                 &entity_link::entity_out_id_, from<entity_link>(),
                 join<shot>(on(c(&entity_link::entity_in_id_) == c(shot->*&entity::uuid_id_))),
                 join<sequence>(on(c(shot->*&entity::parent_id_) == c(sequence->*&entity::uuid_id_))),
                 where(c(sequence->*&entity::uuid_id_) == id_ && !c(shot->*&entity::canceled_))
             ))
      )
  );
  std::map<std::string, std::size_t> l_ass_all_map{};
  for (auto i = 0; i < l_ass_all.size(); ++i) {
    auto&& [l_ass_ext, l_entity_type_id] = l_ass_all[i];
    if (l_entity_type_id == asset_type::get_prop_id()) {
      l_ass_all_map.emplace(
          l_ass_ext.ban_ben_.empty() ? l_ass_ext.pin_yin_ming_cheng_
                                     : fmt::format("{}_{}", l_ass_ext.pin_yin_ming_cheng_, l_ass_ext.ban_ben_),
          i
      );
    } else {
      l_ass_all_map.emplace(l_ass_ext.bian_hao_, i);
    }
  }
  std::vector<uuid> l_ass_shot_ids{};
  std::map<uuid, shot_harvest_t> l_shot_harvest_map;
  for (auto&& [l_shot, l_shot_ext] : l_sql.impl_->storage_any_.select(
           columns(object<entity>(true), object<entity_shot_extend>(true)), from<entity>(),
           join<entity_shot_extend>(on(c(&entity::uuid_id_) == c(&entity_shot_extend::entity_id_))),
           join<sequence>(on(c(&entity::parent_id_) == c(sequence->*&entity::uuid_id_))),
           where(c(sequence->*&entity::uuid_id_) == id_ && !c(&entity::canceled_))
       )) {
    l_ass_shot_ids.emplace_back(l_shot.uuid_id_);
    l_shot_harvest_map.emplace(
        l_shot.uuid_id_, shot_harvest_t{
                             .shot_entity_       = l_shot,
                             .shot_extend_       = l_shot_ext,
                             .sequence_entity_   = &l_seq,
                             .episode_           = &l_episodes,
                             .prj_               = &l_prj,
                             .seq_entts_         = &l_ass_all,
                             .seq_entts_all_map_ = &l_ass_all_map
                         }
    );
  }

  // 查询所有的子镜头和用到的资产
  auto l_ass_link = l_sql.impl_->storage_any_.select(
      columns(
          &entity_link::entity_out_id_, object<entity_asset_extend>(), &entity::entity_type_id_,
          &entity_link::entity_in_id_
      ),
      from<entity_link>(), join<entity>(on(c(&entity::uuid_id_) == c(&entity_link::entity_out_id_))),
      join<entity_asset_extend>(on(c(&entity::uuid_id_) == c(&entity_asset_extend::entity_id_))),
      where(in(&entity_link::entity_in_id_, l_ass_shot_ids))
  );

  for (auto&& [l_entity_out_id, l_asset_ext, l_entity_type_id, l_entity_in_id] : l_ass_link) {
    l_shot_harvest_map.at(l_entity_in_id).shot_entts_.emplace_back(l_entity_out_id, l_asset_ext, l_entity_type_id);
  }

  auto l_install_entity_links = std::make_shared<std::vector<entity_link>>();
  auto l_install_entity       = std::make_shared<std::vector<entity>>();

  for (auto&& [l_key, l_val] : l_shot_harvest_map) l_val(l_install_entity_links, l_install_entity);

  if (!l_install_entity_links->empty()) {
    co_await l_sql.install_range(l_install_entity_links);
    co_await l_sql.update_range(l_install_entity);
    // for (auto&& i : *l_install_entity_links)
    //   socket_io::broadcast(
    //       socket_io::entity_link_new_broadcast_t{
    //           .entity_link_id_ = i.uuid_id_,
    //           .entity_in_id_   = i.entity_in_id_,
    //           .entity_out_id_  = i.entity_out_id_,
    //           .nb_occurences_  = i.nb_occurences_,
    //           .project_id_     = project_id_
    //       }
    //   );
    for (auto&& i : *l_install_entity)
      socket_io::broadcast(
          socket_io::shot_casting_update_broadcast_t{
              .shot_id_ = i.uuid_id_, .project_id_ = project_id_, .nb_entities_out_ = i.nb_entities_out_
          }
      );
  }
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(),
      "用户 {}({}) 完成 UE Assembly 组件收集 project_id {} sequence_id {} harvested_shot_count {} "
      "harvested_assembly_count {} installed_link_count {}",
      person_.person_.email_, person_.person_.get_full_name(), project_id_, id_, l_install_entity_links->size(), 1, 1
  );
  co_return in_handle->make_msg(nlohmann::json{} = get_sequence_casting(project_id_, person_.person_, id_));
}
}  // namespace doodle::http
