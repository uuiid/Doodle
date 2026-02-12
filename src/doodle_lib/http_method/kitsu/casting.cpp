//
// Created by TD on 25-8-21.
//
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/metadata/person.h"
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/http_method/kitsu/kitsu_result.h>

#include <map>
#include <range/v3/range/conversion.hpp>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <sqlite_orm/sqlite_orm.h>
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
  auto l_sql = g_ctx().get<sqlite_database>();
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
  auto l_sql = g_ctx().get<sqlite_database>();
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
  auto l_sql = g_ctx().get<sqlite_database>();

  using namespace sqlite_orm;
  auto l_ret = l_sql.impl_->storage_any_.get_all<entity_link>(where(c(&entity_link::entity_in_id_) == in_entity_id));

  return l_ret;
}

auto get_entity_link_by_entity_id(const std::vector<uuid>& in_entity_id) {
  auto l_sql = g_ctx().get<sqlite_database>();

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
    auto l_sql = g_ctx().get<sqlite_database>();
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
  if (g_ctx().get<sqlite_database>().uuid_to_id<entity>(entity_id_) == 0)
    throw_exception(doodle_error{"实体不存在或已被删除"});
  co_return in_handle->make_msg(nlohmann::json{} = get_casting_t::get(entity_id_));
}

boost::asio::awaitable<boost::beast::http::message_generator> data_project_entities_casting::put(
    session_data_ptr in_handle
) {
  person_.check_in_project(project_id_);
  auto l_sql = g_ctx().get<sqlite_database>();
  auto l_ent = std::make_shared<entity>(l_sql.get_by_uuid<entity>(entity_id_));
  if (l_ent->entity_type_id_ == l_sql.get_entity_type_by_name(std::string{doodle_config::entity_type_episode}).uuid_id_)
    throw_exception(doodle_error{"不能将 Episode 作为实体类型进行操作"});
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
            "entity-link:update",
            nlohmann::json{
                {"entity_link_id", i.uuid_id_}, {"nb_occurences", i.nb_occurences_}, {"project_id", project_id_}
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
          socket_io::broadcast("asset:update", nlohmann::json{{"asset_id", id}, {"project_id", project_id_}});
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
            "entity-link:new", nlohmann::json{
                                   {"entity_link_id", i.uuid_id_},
                                   {"entity_in_id", i.entity_in_id_},
                                   {"entity_out_id", i.entity_out_id_},
                                   {"nb_occurences", i.nb_occurences_},
                                   {"project_id", project_id_}
                               }
        );
      });
    }
  }
  if (auto l_id_list = l_shot_links | ranges::views::transform(&entity_link::id_) | ranges::to_vector;
      !l_id_list.empty()) {
    co_await l_sql.remove<entity_link>(l_id_list);
    SPDLOG_LOGGER_WARN(in_handle->logger_, "移除实体链接 {}", fmt::join(l_id_list, ", "));
  }

  l_ent->nb_entities_out_ = l_list.size();
  co_await l_sql.update(l_ent);
  if (!l_entity_links_update->empty()) {
    co_await l_sql.update_range(l_entity_links_update);
    SPDLOG_LOGGER_WARN(in_handle->logger_, "更新实体链接 {}", l_entity_links_update->size());
  }
  if (!l_entity_links_insert->empty()) {
    co_await l_sql.install_range(l_entity_links_insert);
    SPDLOG_LOGGER_WARN(in_handle->logger_, "新增实体链接 {}", l_entity_links_insert->size());
  }
  // for (auto&& i : l_delay_events) i();
  socket_io::broadcast(
      "shot:casting-update",
      nlohmann::json{
          {"shot_id", l_ent->uuid_id_}, {"project_id", project_id_}, {"nb_entities_out", l_ent->nb_entities_out_}
      }
  );
  co_return in_handle->make_msg(in_handle->get_json());
}
boost::asio::awaitable<boost::beast::http::message_generator> data_project_sequences_casting::get(
    session_data_ptr in_handle
) {
  person_.check_in_project(project_id_);
  person_.check_not_outsourcer();
  if (g_ctx().get<sqlite_database>().uuid_to_id<entity>(sequence_id_) == 0)
    throw_exception(doodle_error{"序列不存在或已被删除"});
  co_return in_handle->make_msg(nlohmann::json{} = get_sequence_casting(project_id_, person_.person_, sequence_id_));
}
boost::asio::awaitable<boost::beast::http::message_generator> data_project_asset_types_casting::get(
    session_data_ptr in_handle
) {
  person_.check_in_project(project_id_);
  person_.check_not_outsourcer();
  if (g_ctx().get<sqlite_database>().uuid_to_id<asset_type>(asset_type_id_) == 0)
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
      g_logger_ctrl().get_http(), "用户 {}({}) 开始替换 Casting project_id {} item_count {}",
      person_.person_.email_, person_.person_.get_full_name(), project_id_, l_list.size()
  );
  auto l_sql                                               = g_ctx().get<sqlite_database>();
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
          "entity-link:update", nlohmann::json{
                                    {"entity_link_id", l_link->uuid_id_},
                                    {"nb_occurences", l_link->nb_occurences_},
                                    {"project_id", project_id_}
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

}  // namespace doodle::http
