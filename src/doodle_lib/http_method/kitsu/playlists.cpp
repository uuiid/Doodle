//
// Created by TD on 25-5-14.
//
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/person.h"
#include "doodle_core/metadata/preview_file.h"
#include "doodle_core/metadata/task.h"
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/playlist.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include "core/http/http_function.h"
#include <spdlog/spdlog.h>
#include <vector>

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> playlists_entities_preview_files::get(
    session_data_ptr in_handle
) {
  auto l_sql     = g_ctx().get<sqlite_database>();
  auto l_entt_id = l_sql.get_by_uuid<entity>(id_);
  person_.check_not_outsourcer();
  person_.check_in_project(l_entt_id.project_id_);

  nlohmann::json l_json{nlohmann::json::object()};
  for (auto&& [key, value] : l_sql.get_preview_files_for_entity(l_entt_id.uuid_id_))
    l_json[fmt::to_string(key)] = value;
  co_return in_handle->make_msg(l_json);
}
namespace {
struct actions_preview_files_update_annotations_args {
  std::vector<preview_file::annotations_t> additions_;
  std::vector<preview_file::annotations_t> updates_;
  std::vector<preview_file::annotations_t> deletions_;
  // from json
  friend void from_json(const nlohmann::json& j, actions_preview_files_update_annotations_args& p) {
    if (j.at("additions").is_array()) j.at("additions").get_to(p.additions_);
    if (j.at("updates").is_array()) j.at("updates").get_to(p.updates_);
    if (j.at("deletions").is_array()) j.at("deletions").get_to(p.deletions_);
  }
};
}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> actions_preview_files_update_annotations::put(
    session_data_ptr in_handle
) {
  person_.check_not_outsourcer();
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_prev = std::make_shared<preview_file>(l_sql.get_by_uuid<preview_file>(preview_file_id_));
  auto l_task = l_sql.get_by_uuid<task>(l_prev->task_id_);
  person_.check_in_project(l_task.project_id_);

  auto l_args = in_handle->get_json().get<actions_preview_files_update_annotations_args>();
  std::map<std::double_t, preview_file::annotations_t> l_time_map{};
  for (auto&& i : l_prev->get_annotations()) l_time_map[i.time_] = std::move(i);

  for (auto&& i : l_args.additions_) {
    for (auto&& j : i.objects_) j.id_ = core_set::get_set().get_uuid();
    if (!l_time_map.contains(i.time_))
      l_time_map.emplace(i.time_, std::move(i));
    else
      l_time_map[i.time_].objects_.insert(l_time_map[i.time_].objects_.end(), i.objects_.begin(), i.objects_.end());
  }

  for (auto&& i : l_args.updates_) {
    if (!l_time_map.contains(i.time_)) continue;
    std::map<uuid, std::size_t> l_id_map{};
    auto& l_objs = l_time_map[i.time_].objects_;
    for (auto j = 0; j < l_objs.size(); ++j) l_id_map[l_objs[j].id_] = j;
    for (auto&& j : i.objects_) {
      if (l_id_map.contains(j.id_))
        l_objs[l_id_map[j.id_]] = std::move(j);
      else
        l_objs.emplace_back(std::move(j));
    }
  }
  for (auto&& i : l_args.deletions_) {
    if (!l_time_map.contains(i.time_)) continue;
    std::set<uuid> l_ids{};
    for (auto&& j : i.objects_) l_ids.insert(j.id_);
    auto& l_objs = l_time_map[i.time_].objects_;
    l_objs |= ranges::actions::remove_if([&l_ids](const auto& j) { return l_ids.contains(j.id_); });
  }
  auto l_ru = l_time_map | ranges::views::values | ranges::to_vector;
  l_ru |= ranges::actions::remove_if([](const auto& j) { return j.objects_.empty(); });
  l_prev->set_annotations(std::move(l_ru));

  co_await l_sql.update(l_prev);
  socket_io::broadcast(
      "preview-file:annotation-update",
      nlohmann::json{
          {"preview_file_id", l_prev->uuid_id_},
          {"person_id", person_.person_.uuid_id_},
          {"updated_at", l_prev->updated_at_},
          {"project_id", l_task.project_id_}
      },
      "/events"
  );
  co_return in_handle->make_msg(nlohmann::json{} = *l_prev);
}
namespace {
struct base_playlist_t {
  explicit base_playlist_t(
      const entity& in_entt, const uuid& in_task_id,
      const std::map<uuid, std::vector<preview_files_for_entity_t>>& in_v, const uuid& in_task_type_id
  )
      : id_(in_entt.uuid_id_), name_(in_entt.name_), preview_file_task_id_(in_task_id), preview_files_(in_v) {
    if (!in_task_type_id.is_nil() && preview_files_.map_.contains(in_task_type_id) &&
        !preview_files_.map_.at(in_task_type_id).empty()) {
      auto&& l_pf                 = preview_files_.map_.at(in_task_type_id).front();
      preview_file_id_            = l_pf.uuid_id_;
      preview_file_extension_     = l_pf.extension_;
      preview_file_width_         = l_pf.width_;
      preview_file_height_        = l_pf.height_;
      preview_file_duration_      = l_pf.duration_;
      preview_file_revision_      = l_pf.revision_;
      preview_file_status_        = l_pf.status_;
      preview_file_source_        = l_pf.source_;
      preview_file_annotations_   = l_pf.annotations_;
      preview_file_previews_list_ = l_pf.previews_;
    }
  }

  struct preview_files_for_entity_map_t {
    std::map<uuid, std::vector<preview_files_for_entity_t>> map_;
    // to json
    friend void to_json(nlohmann::json& j, const preview_files_for_entity_map_t& p) {
      if (p.map_.empty()) j = nlohmann::json::array();
      for (auto&& [key, value] : p.map_) j[fmt::to_string(key)] = value;
    }
  };

  uuid id_;
  std::string name_;
  uuid preview_file_task_id_;
  std::string parent_name_;

  friend bool operator<(const base_playlist_t& lhs, const base_playlist_t& rhs) { return lhs.name_ < rhs.name_; }
  friend bool operator<=(const base_playlist_t& lhs, const base_playlist_t& rhs) { return !(rhs < lhs); }
  friend bool operator>(const base_playlist_t& lhs, const base_playlist_t& rhs) { return rhs < lhs; }
  friend bool operator>=(const base_playlist_t& lhs, const base_playlist_t& rhs) { return !(lhs < rhs); }

  decltype(preview_file::uuid_id_) preview_file_id_;
  decltype(preview_file::extension_) preview_file_extension_;
  decltype(preview_file::width_) preview_file_width_;
  decltype(preview_file::height_) preview_file_height_;
  decltype(preview_file::duration_) preview_file_duration_;
  decltype(preview_file::revision_) preview_file_revision_;
  decltype(preview_file::status_) preview_file_status_;
  decltype(preview_file::source_) preview_file_source_;
  decltype(preview_file::annotations_) preview_file_annotations_;
  preview_files_for_entity_map_t preview_files_;
  std::vector<preview_files_for_entity_t> preview_file_previews_list_;

  // to json
  friend void to_json(nlohmann::json& j, const base_playlist_t& p) {
    j["id"]                       = p.id_;
    j["name"]                     = p.name_;
    j["preview_file_task_id"]     = p.preview_file_task_id_;
    j["parent_name"]              = p.parent_name_;

    j["preview_file_id"]          = p.preview_file_id_;
    j["preview_file_extension"]   = p.preview_file_extension_;
    j["preview_file_width"]       = p.preview_file_width_;
    j["preview_file_height"]      = p.preview_file_height_;
    j["preview_file_duration"]    = p.preview_file_duration_;
    j["preview_file_revision"]    = p.preview_file_revision_;
    j["preview_file_status"]      = p.preview_file_status_;
    j["preview_file_source"]      = p.preview_file_source_;
    j["preview_file_annotations"] = p.preview_file_annotations_;
    j["preview_file_previews"]    = p.preview_file_previews_list_;

    j["preview_files"]            = p.preview_files_;
  }
};
struct shot_for_playlist_t : base_playlist_t {
  explicit shot_for_playlist_t(
      const entity& in_seq_ent, const entity& in_shot, const uuid& in_task_id,
      const std::map<uuid, std::vector<preview_files_for_entity_t>>& in_v, const uuid& in_task_type_id

  )
      : base_playlist_t(in_shot, in_task_id, in_v, in_task_type_id),
        sequence_id_(in_seq_ent.uuid_id_),
        sequence_name_(in_seq_ent.name_) {
    parent_name_ = in_seq_ent.name_;
  }
  friend bool operator<(const shot_for_playlist_t& lhs, const shot_for_playlist_t& rhs) {
    return lhs.name_ < rhs.name_;
  }
  friend bool operator<=(const shot_for_playlist_t& lhs, const shot_for_playlist_t& rhs) { return !(rhs < lhs); }
  friend bool operator>(const shot_for_playlist_t& lhs, const shot_for_playlist_t& rhs) { return rhs < lhs; }
  friend bool operator>=(const shot_for_playlist_t& lhs, const shot_for_playlist_t& rhs) { return !(lhs < rhs); }

  uuid sequence_id_;
  std::string sequence_name_;
  // to_json
  friend void to_json(nlohmann::json& j, const shot_for_playlist_t& p) {
    to_json(j, static_cast<const base_playlist_t&>(p));
    j["sequence_id"]   = p.sequence_id_;
    j["sequence_name"] = p.sequence_name_;
  }
};
struct asset_for_playlist_t : base_playlist_t {
  explicit asset_for_playlist_t(
      const asset_type& in_ass_type, const entity& in_entt, const uuid& in_task_id,
      const std::map<uuid, std::vector<preview_files_for_entity_t>>& in_v, const uuid& in_task_type_id
  )
      : base_playlist_t(in_entt, in_task_id, in_v, in_task_type_id),
        asset_type_id_(in_ass_type.uuid_id_),
        asset_type_name_(in_ass_type.name_) {
    parent_name_ = asset_type_name_;
  }
  uuid asset_type_id_;
  std::string asset_type_name_;

  friend bool operator<(const asset_for_playlist_t& lhs, const asset_for_playlist_t& rhs) {
    return lhs.asset_type_name_ < rhs.asset_type_name_;
  }
  friend bool operator<=(const asset_for_playlist_t& lhs, const asset_for_playlist_t& rhs) { return !(rhs < lhs); }
  friend bool operator>(const asset_for_playlist_t& lhs, const asset_for_playlist_t& rhs) { return rhs < lhs; }
  friend bool operator>=(const asset_for_playlist_t& lhs, const asset_for_playlist_t& rhs) { return !(lhs < rhs); }

  // to_json
  friend void to_json(nlohmann::json& j, const asset_for_playlist_t& p) {
    to_json(j, static_cast<const base_playlist_t&>(p));
    j["asset_type_id"]   = p.asset_type_id_;
    j["asset_type_name"] = p.asset_type_name_;
  }
};
struct sequence_for_playlist_t : base_playlist_t {
  explicit sequence_for_playlist_t(
      const entity& in_seq_ent, const uuid& in_task_id,
      const std::map<uuid, std::vector<preview_files_for_entity_t>>& in_v, const uuid& in_task_type_id
  )
      : base_playlist_t(in_seq_ent, in_task_id, in_v, in_task_type_id) {}

  friend bool operator<(const sequence_for_playlist_t& lhs, const sequence_for_playlist_t& rhs) {
    return static_cast<const base_playlist_t&>(lhs) < static_cast<const base_playlist_t&>(rhs);
  }
  friend bool operator<=(const sequence_for_playlist_t& lhs, const sequence_for_playlist_t& rhs) {
    return !(rhs < lhs);
  }
  friend bool operator>(const sequence_for_playlist_t& lhs, const sequence_for_playlist_t& rhs) { return rhs < lhs; }
  friend bool operator>=(const sequence_for_playlist_t& lhs, const sequence_for_playlist_t& rhs) {
    return !(lhs < rhs);
  }

  // uuid sequence_id_;
  // std::string sequence_name_;
  // to_json
  friend void to_json(nlohmann::json& j, const sequence_for_playlist_t& p) {
    to_json(j, static_cast<const base_playlist_t&>(p));
    j["sequence_id"]   = "";
    j["sequence_name"] = "";
  }
};
struct sequence_episode_for_playlist_t : base_playlist_t {
  explicit sequence_episode_for_playlist_t(
      const entity& in_episode_ent, const entity& in_seq_ent, const uuid& in_task_id,
      const std::map<uuid, std::vector<preview_files_for_entity_t>>& in_v, const uuid& in_task_type_id
  )
      : base_playlist_t(in_seq_ent, in_task_id, in_v, in_task_type_id),
        episode_id_(in_episode_ent.uuid_id_),
        episode_name_(in_episode_ent.name_) {
    parent_name_ = in_episode_ent.name_;
  }
  uuid episode_id_;
  std::string episode_name_;

  friend bool operator<(const sequence_episode_for_playlist_t& lhs, const sequence_episode_for_playlist_t& rhs) {
    return lhs.episode_name_ < rhs.episode_name_;
  }
  friend bool operator<=(const sequence_episode_for_playlist_t& lhs, const sequence_episode_for_playlist_t& rhs) {
    return !(rhs < lhs);
  }
  friend bool operator>(const sequence_episode_for_playlist_t& lhs, const sequence_episode_for_playlist_t& rhs) {
    return rhs < lhs;
  }
  friend bool operator>=(const sequence_episode_for_playlist_t& lhs, const sequence_episode_for_playlist_t& rhs) {
    return !(lhs < rhs);
  }

  // to_json
  friend void to_json(nlohmann::json& j, const sequence_episode_for_playlist_t& p) {
    to_json(j, static_cast<const base_playlist_t&>(p));
    j["episode_id"]   = p.episode_id_;
    j["episode_name"] = p.episode_name_;
  }
};
struct episode_for_playlist_t : base_playlist_t {
  explicit episode_for_playlist_t(
      const entity& in_episode_ent, const uuid& in_task_id,
      const std::map<uuid, std::vector<preview_files_for_entity_t>>& in_v, const uuid& in_task_type_id
  )
      : base_playlist_t(in_episode_ent, in_task_id, in_v, in_task_type_id) {}

  friend bool operator<(const episode_for_playlist_t& lhs, const episode_for_playlist_t& rhs) {
    return static_cast<const base_playlist_t&>(lhs) < static_cast<const base_playlist_t&>(rhs);
  }
  friend bool operator<=(const episode_for_playlist_t& lhs, const episode_for_playlist_t& rhs) { return !(rhs < lhs); }
  friend bool operator>(const episode_for_playlist_t& lhs, const episode_for_playlist_t& rhs) { return rhs < lhs; }
  friend bool operator>=(const episode_for_playlist_t& lhs, const episode_for_playlist_t& rhs) { return !(lhs < rhs); }

  // uuid sequence_id_;
  // std::string sequence_name_;
  // to_json
  friend void to_json(nlohmann::json& j, const episode_for_playlist_t& p) {
    to_json(j, static_cast<const base_playlist_t&>(p));
    j["sequence_id"]   = "";
    j["sequence_name"] = "";
  }
};
struct temp_playlist_t {
  using playlist_variant_t = std::variant<
      shot_for_playlist_t, asset_for_playlist_t, sequence_for_playlist_t, sequence_episode_for_playlist_t,
      episode_for_playlist_t>;
  std::vector<playlist_variant_t> playlist_;

  // to_json
  friend void to_json(nlohmann::json& j, const temp_playlist_t& p) {
    j = nlohmann::json::array();
    for (auto&& l_v : p.playlist_) {
      std::visit([&](auto&& arg) { to_json(j.emplace_back(), arg); }, l_v);
    }
  }
};

}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> data_project_playlists_temp::post(
    session_data_ptr in_handle
) {
  person_.check_not_outsourcer();
  person_.check_in_project(project_id_);
  auto l_sql      = g_ctx().get<sqlite_database>();
  auto l_task_ids = in_handle->get_json().value("task_ids", std::vector<uuid>{});
  std::vector<task> l_tasks{};
  l_tasks.reserve(l_task_ids.size());
  for (auto&& i : l_task_ids) l_tasks.emplace_back(l_sql.get_by_uuid<task>(i));
  std::vector<entity> l_entts{};
  l_entts.reserve(l_tasks.size());
  for (auto&& i : l_tasks) l_entts.emplace_back(l_sql.get_by_uuid<entity>(i.entity_id_));
  bool l_is_sort{};
  for (auto&& [key, value, has] : in_handle->url_.params())
    if (key == "sort") l_is_sort = true;

  temp_playlist_t l_playlist{};
  for (auto&& i : l_entts) {
    if (i.entity_type_id_ == asset_type::get_shot_id()) {
      l_playlist.playlist_.emplace_back(
          shot_for_playlist_t{
              l_sql.get_by_uuid<entity>(i.parent_id_), i, l_tasks[0].uuid_id_,
              l_sql.get_preview_files_for_entity(i.uuid_id_), l_tasks[0].task_type_id_
          }
      );
    } else if (i.entity_type_id_ == asset_type::get_sequence_id()) {
      if (i.parent_id_.is_nil())
        l_playlist.playlist_.emplace_back(
            sequence_for_playlist_t{
                i, l_tasks[0].uuid_id_, l_sql.get_preview_files_for_entity(i.uuid_id_), l_tasks[0].task_type_id_
            }
        );
      else
        l_playlist.playlist_.emplace_back(
            sequence_episode_for_playlist_t{
                l_sql.get_by_uuid<entity>(i.parent_id_), i, l_tasks[0].uuid_id_,
                l_sql.get_preview_files_for_entity(i.uuid_id_), l_tasks[0].task_type_id_
            }
        );

    } else if (i.entity_type_id_ == asset_type::get_episode_id()) {
      l_playlist.playlist_.emplace_back(
          episode_for_playlist_t{
              i, l_tasks[0].uuid_id_, l_sql.get_preview_files_for_entity(i.uuid_id_), l_tasks[0].task_type_id_
          }
      );
    } else {
      l_playlist.playlist_.emplace_back(
          asset_for_playlist_t{
              l_sql.get_by_uuid<asset_type>(i.entity_type_id_), i, l_tasks[0].uuid_id_,
              l_sql.get_preview_files_for_entity(i.uuid_id_), l_tasks[0].task_type_id_
          }
      );
    }
  }
  if (l_is_sort)
    std::sort(
        l_playlist.playlist_.begin(), l_playlist.playlist_.end(),
        [](const temp_playlist_t::playlist_variant_t& l_l, const temp_playlist_t::playlist_variant_t& r_r) -> bool {
          return std::visit(
              [](auto&& l_l, auto&& r_r) {
                if constexpr (std::is_same_v<std::decay_t<decltype(l_l)>, std::decay_t<decltype(r_r)>>) {
                  return l_l < r_r;
                } else {
                  return false;
                }
              },
              l_l, r_r
          );
        }
    );

  co_return in_handle->make_msg(nlohmann::json{} = l_playlist);
}
boost::asio::awaitable<boost::beast::http::message_generator> data_project_playlists::get(session_data_ptr in_handle) {
  person_.check_in_project(project_id_);
  person_.check_not_outsourcer();
  auto l_sql = g_ctx().get<sqlite_database>();
  std::int32_t l_page{1};
  uuid l_task_type_id{};
  std::string l_order_by{};
  for (auto&& [key, value, has] : in_handle->url_.params()) {
    if (key == "page" && has) l_page = std::clamp(std::stoi(value), 1, INT32_MAX);
    if (key == "task_type_id" && has) l_task_type_id = from_uuid_str(value);
    if (key == "order_by" && has) l_order_by = value;
  }
  std::int32_t l_offset = (l_page - 1) * 20;

  using namespace sqlite_orm;
  auto l_order = dynamic_order_by(l_sql.impl_->storage_any_);
  if (l_order_by == "create_at") {
    l_order.push_back(order_by(&playlist::created_at_).desc());
  } else if (l_order_by == "name") {
    l_order.push_back(order_by(&playlist::name_));
  } else {
    l_order.push_back(order_by(&playlist::updated_at_).desc());
  }
  auto l_playlists = l_sql.impl_->storage_any_.get_all<playlist>(
      where(
          c(&playlist::project_id_) == project_id_ &&
          (l_task_type_id.is_nil() || c(&playlist::task_type_id_) == l_task_type_id)
      ),
      l_order, limit(l_offset, 20)
  );
  co_return in_handle->make_msg(nlohmann::json{} = l_playlists);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_playlists::post(session_data_ptr in_handle) {
  auto l_sql      = g_ctx().get<sqlite_database>();
  auto l_playlist = std::make_shared<playlist>();
  in_handle->get_json().get_to(*l_playlist);
  person_.check_in_project(l_playlist->project_id_);
  person_.check_not_outsourcer();
  default_logger_raw()->info("{} 创建播放列表 {}", person_.person_.email_, l_playlist->name_);
  co_await l_sql.install(l_playlist);
  co_return in_handle->make_msg(nlohmann::json{} = *l_playlist);
}

struct playlist_shot_t : playlist {
  struct preview_file_mini_t {
    explicit preview_file_mini_t(const preview_file& in_preview_file)
        : id_{in_preview_file.uuid_id_},
          revision_{in_preview_file.revision_},
          extension_{in_preview_file.extension_},
          width_{in_preview_file.width_},
          height_{in_preview_file.height_},
          duration_{in_preview_file.duration_},
          status_{in_preview_file.status_},
          annotations_{in_preview_file.annotations_},
          task_id_{in_preview_file.task_id_},
          preview_file_source_{in_preview_file.source_} {}

    uuid id_;
    std::int32_t revision_;
    std::string extension_;
    std::int32_t width_;
    std::int32_t height_;
    std::double_t duration_;
    preview_file_statuses status_;
    nlohmann::json annotations_;
    uuid task_id_;
    decltype(preview_file::source_) preview_file_source_;

    // to json
    friend void to_json(nlohmann::json& j, const preview_file_mini_t& l_preview_file_mini) {
      j["id"]          = l_preview_file_mini.id_;
      j["revision"]    = l_preview_file_mini.revision_;
      j["extension"]   = l_preview_file_mini.extension_;
      j["width"]       = l_preview_file_mini.width_;
      j["height"]      = l_preview_file_mini.height_;
      j["duration"]    = l_preview_file_mini.duration_;
      j["status"]      = l_preview_file_mini.status_;
      j["annotations"] = l_preview_file_mini.annotations_;
      j["task_id"]     = l_preview_file_mini.task_id_;
    }
  };

  struct playlist_shot_entity_t {
    explicit playlist_shot_entity_t(const uuid& in_entity_id)
        : entity_id_{in_entity_id},
          preview_file_id_{},
          id_{in_entity_id},
          preview_files_{},
          preview_file_extension_{},
          preview_file_revision_{},
          preview_file_width_{},
          preview_file_height_{},
          preview_file_duration_(),
          preview_file_status_(),
          preview_file_annotations_(),
          preview_file_task_id_() {}
    explicit playlist_shot_entity_t(
        const uuid& in_entity_id, const preview_file& in_preview_file, const playlist_shot& in_playlist_shot
    )
        : entity_id_{in_entity_id},
          preview_file_id_{in_preview_file.uuid_id_},
          id_{in_playlist_shot.uuid_id_},
          preview_files_{},
          preview_file_extension_{in_preview_file.extension_},
          preview_file_revision_{in_preview_file.revision_},
          preview_file_width_{in_preview_file.width_},
          preview_file_height_{in_preview_file.height_},
          preview_file_duration_{in_preview_file.duration_},
          preview_file_status_{in_preview_file.status_},
          preview_file_source_{in_preview_file.source_},
          preview_file_annotations_{in_preview_file.annotations_},
          preview_file_task_id_{in_preview_file.task_id_},
          order_index_{in_playlist_shot.order_index_},
          shot_id_{in_playlist_shot.uuid_id_}

    {}

    playlist_shot_entity_t& update(const preview_file& in_preview_file) {
      preview_file_id_          = in_preview_file.uuid_id_;
      preview_file_extension_   = in_preview_file.extension_;
      preview_file_revision_    = in_preview_file.revision_;
      preview_file_width_       = in_preview_file.width_;
      preview_file_height_      = in_preview_file.height_;
      preview_file_duration_    = in_preview_file.duration_;
      preview_file_status_      = in_preview_file.status_;
      preview_file_source_      = in_preview_file.source_;
      preview_file_annotations_ = in_preview_file.annotations_;
      preview_file_task_id_     = in_preview_file.task_id_;
      return *this;
    }
    playlist_shot_entity_t& update(const playlist_shot& in_playlist_shot) {
      order_index_ = in_playlist_shot.order_index_;
      shot_id_     = in_playlist_shot.uuid_id_;
      return *this;
    }

    uuid entity_id_;
    uuid preview_file_id_;
    uuid id_;
    std::map<uuid, std::vector<preview_file_mini_t>> preview_files_;
    std::string preview_file_extension_;
    std::int32_t preview_file_revision_;
    std::int32_t preview_file_width_;
    std::int32_t preview_file_height_;
    decltype(preview_file::duration_) preview_file_duration_;
    preview_file_statuses preview_file_status_;
    decltype(preview_file::source_) preview_file_source_;
    nlohmann::json preview_file_annotations_;
    uuid preview_file_task_id_;
    std::int32_t order_index_;
    uuid shot_id_;
    // to json
    friend void to_json(nlohmann::json& j, const playlist_shot_entity_t& l_playlist_shot_entity) {
      j["entity_id"]   = l_playlist_shot_entity.entity_id_;
      j["id"]          = l_playlist_shot_entity.id_;
      j["shot_id"]     = l_playlist_shot_entity.shot_id_;
      j["order_index"] = l_playlist_shot_entity.order_index_;

      for (auto&& [preview_file_id, preview_files] : l_playlist_shot_entity.preview_files_) {
        j["preview_files"][fmt::to_string(preview_file_id)] = preview_files;
      }
      if (l_playlist_shot_entity.preview_file_id_.is_nil()) return;
      j["preview_file_id"]          = l_playlist_shot_entity.preview_file_id_;
      j["preview_file_extension"]   = l_playlist_shot_entity.preview_file_extension_;
      j["preview_file_revision"]    = l_playlist_shot_entity.preview_file_revision_;
      j["preview_file_width"]       = l_playlist_shot_entity.preview_file_width_;
      j["preview_file_height"]      = l_playlist_shot_entity.preview_file_height_;
      j["preview_file_duration"]    = l_playlist_shot_entity.preview_file_duration_;
      j["preview_file_status"]      = l_playlist_shot_entity.preview_file_status_;
      j["preview_file_source"]      = l_playlist_shot_entity.preview_file_source_;
      j["preview_file_annotations"] = l_playlist_shot_entity.preview_file_annotations_;
      j["preview_file_task_id"]     = l_playlist_shot_entity.preview_file_task_id_;
    }
  };

  explicit playlist_shot_t(const playlist& in_playlist) : playlist(in_playlist) {}

  std::vector<playlist_shot_entity_t> shot_;
  // to json
  friend void to_json(nlohmann::json& j, const playlist_shot_t& l_playlist_shot) {
    to_json(j, static_cast<const playlist&>(l_playlist_shot));
    j["shots"]      = l_playlist_shot.shot_;
    j["build_jobs"] = nlohmann::json::array();
  }
};

auto get_playlist_shot_entity(const playlist& in_playlist) {
  auto l_sql                 = g_ctx().get<sqlite_database>();
  const auto l_playlist_shot = l_sql.get_playlist_shot_entity(in_playlist.uuid_id_);
  std::vector<uuid> l_entity_ids{};
  std::map<uuid, const playlist_shot*> l_playlist_shot_map{};
  l_entity_ids.reserve(l_playlist_shot.size());
  for (auto&& i : l_playlist_shot) {
    l_entity_ids.emplace_back(i.entity_id_);
    l_playlist_shot_map.emplace(i.entity_id_, &i);
  }
  playlist_shot_t l_ret{in_playlist};
  std::map<uuid, std::size_t> l_entity_id_to_index{};

  for (std::size_t i = 0; i < l_playlist_shot.size(); ++i) {
    if (!l_entity_id_to_index.contains(l_playlist_shot.at(i).entity_id_)) {
      l_ret.shot_.emplace_back(playlist_shot_t::playlist_shot_entity_t{l_playlist_shot.at(i).entity_id_})
          .update(l_playlist_shot.at(i));
      l_entity_id_to_index.emplace(l_playlist_shot.at(i).entity_id_, l_ret.shot_.size() - 1);
    }
  };

  using namespace sqlite_orm;
  for (auto&& [l_preview_file, l_task_type_id, l_entity_id] : l_sql.impl_->storage_any_.select(
           columns(object<preview_file>(true), &task::task_type_id_, &task::entity_id_),
           join<task>(on(c(&task::uuid_id_) == c(&preview_file::task_id_))),
           join<task_type>(on(c(&task::task_type_id_) == c(&task_type::uuid_id_))),
           where(in(&task::entity_id_, l_entity_ids)),
           multi_order_by(
               order_by(&task_type::priority_).desc(), order_by(&task_type::name_),
               order_by(&preview_file::revision_).desc(), order_by(&preview_file::position_),
               order_by(&preview_file::created_at_)
           )
       )) {
    DOODLE_CHICK(
        l_playlist_shot_map.contains(l_entity_id),
        fmt::format("Preview file's entity {} is not in playlist {}", l_entity_id, in_playlist.uuid_id_)
    );
    l_ret.shot_.at(l_entity_id_to_index.at(l_entity_id))
        .preview_files_[l_task_type_id]
        .emplace_back(playlist_shot_t::preview_file_mini_t{l_preview_file});
    if (l_playlist_shot_map.at(l_entity_id)->preview_id_ == l_preview_file.uuid_id_) {
      l_ret.shot_.at(l_entity_id_to_index.at(l_entity_id)).update(l_preview_file);
    }
  }
  return l_ret;
}

boost::asio::awaitable<boost::beast::http::message_generator> data_project_playlists_instance::get(
    session_data_ptr in_handle
) {
  person_.check_in_project(project_id_);
  person_.check_not_outsourcer();
  auto l_sql      = g_ctx().get<sqlite_database>();
  auto l_playlist = l_sql.get_by_uuid<playlist>(playlist_id_);
  co_return in_handle->make_msg(nlohmann::json{} = get_playlist_shot_entity(l_playlist));
}
boost::asio::awaitable<boost::beast::http::message_generator> data_playlists_instance::put(session_data_ptr in_handle) {
  auto l_sql      = g_ctx().get<sqlite_database>();
  auto l_playlist = std::make_shared<playlist>(l_sql.get_by_uuid<playlist>(id_));
  person_.check_in_project(l_playlist->project_id_);
  person_.check_not_outsourcer();
  auto l_json = in_handle->get_json();
  l_json.get_to(*l_playlist);
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "{}({}) 修改播放列表 {}", person_.person_.email_, person_.person_.get_full_name(),
      l_playlist->name_
  );
  co_await l_sql.update(l_playlist);
  nlohmann::json l_ret{};
  l_ret         = *l_playlist;
  l_ret["shot"] = l_sql.get_playlist_shot_entity(l_playlist->uuid_id_);
  co_return in_handle->make_msg(l_ret);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_playlists_instance, delete_) {
  auto l_sql      = g_ctx().get<sqlite_database>();
  auto l_playlist = l_sql.get_by_uuid<playlist>(id_);
  person_.check_in_project(l_playlist.project_id_);
  person_.check_not_outsourcer();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "{}({}) 删除播放列表 {}", person_.person_.email_, person_.person_.get_full_name(),
      l_playlist.name_
  );
  co_await l_sql.remove<playlist>(l_playlist.id_);
  co_return in_handle->make_msg_204();
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_playlists_instance_entity_instance, post) {
  auto l_sql      = g_ctx().get<sqlite_database>();
  auto l_playlist = std::make_shared<playlist>(l_sql.get_by_uuid<playlist>(playlist_id_));
  auto l_entity   = l_sql.get_by_uuid<entity>(entity_id_);
  person_.check_in_project(l_playlist->project_id_);
  person_.check_not_outsourcer();
  auto l_json = in_handle->get_json();
  SPDLOG_LOGGER_WARN(
      in_handle->logger_, "{} 在播放列表 {} 中添加预览文件 {}", person_.person_.email_, l_playlist->name_,
      l_entity.uuid_id_
  );
  std::shared_ptr<playlist_shot> l_playlist_shot = std::make_shared<playlist_shot>();
  l_json.get_to(*l_playlist_shot);
  l_playlist_shot->playlist_id_ = playlist_id_;
  l_playlist_shot->entity_id_   = l_entity.uuid_id_;
  using namespace sqlite_orm;
  if (l_playlist_shot->order_index_ <= 0)  // 如果没有指定顺序，则放在最后
    l_playlist_shot->order_index_ =
        l_sql.impl_->storage_any_.count<playlist_shot>(where(c(&playlist_shot::playlist_id_) == playlist_id_)) * 100;
  co_await l_sql.install(l_playlist_shot);
  co_return in_handle->make_msg(nlohmann::json{} = *l_playlist_shot);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_playlists_instance_shots, put) {
  auto l_sql      = g_ctx().get<sqlite_database>();
  auto l_playlist = l_sql.get_by_uuid<playlist>(playlist_id_);
  person_.check_in_project(l_playlist.project_id_);
  person_.check_not_outsourcer();
  SPDLOG_LOGGER_WARN(
      in_handle->logger_, "{} 修改播放列表 {} 中的镜头 {}", person_.person_.email_, l_playlist.name_, shot_id_
  );
  auto l_json          = in_handle->get_json();
  auto l_playlist_shot = std::make_shared<playlist_shot>(l_sql.get_by_uuid<playlist_shot>(shot_id_));
  DOODLE_CHICK(*l_playlist_shot, "Preview file is not in playlist");
  l_json.get_to(*l_playlist_shot);
  l_playlist_shot->playlist_id_ = playlist_id_;
  co_await l_sql.update(l_playlist_shot);
  co_return in_handle->make_msg(nlohmann::json{} = *l_playlist_shot);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_playlists_instance_shots, delete_) {
  auto l_sql      = g_ctx().get<sqlite_database>();
  auto l_playlist = l_sql.get_by_uuid<playlist>(playlist_id_);
  person_.check_in_project(l_playlist.project_id_);
  person_.check_not_outsourcer();
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "{}({}) 删除播放列表 {} 中的镜头 {}", person_.person_.email_,
      person_.person_.get_full_name(), l_playlist.name_, shot_id_
  );
  co_await l_sql.remove<playlist_shot>(shot_id_);
  co_return in_handle->make_msg_204();
}
namespace {
struct get_sequence_shots_preview_t : boost::less_than_comparable<get_sequence_shots_preview_t> {
  uuid shot_id_;
  preview_file preview_;
  std::string name_;
  explicit get_sequence_shots_preview_t(
      const uuid& in_shot_id, const preview_file& in_preview, const std::string& in_name
  )
      : shot_id_(in_shot_id), preview_(in_preview), name_(in_name) {}

  bool operator<(const get_sequence_shots_preview_t& other) const { return name_ < other.name_; }
  bool operator==(const get_sequence_shots_preview_t& other) const { return name_ == other.name_; }
};
auto get_sequence_shots_preview(const uuid& in_sequence_id) {
  auto l_sql = g_ctx().get<sqlite_database>();
  using namespace sqlite_orm;

  constexpr auto sequence = "sequence"_alias.for_<entity>();
  constexpr auto episode  = "episode"_alias.for_<entity>();
  auto l_shots            = l_sql.impl_->storage_any_.select(
      columns(object<preview_file>(true), &entity::uuid_id_, &entity::name_), from<preview_file>(),
      join<task>(on(c(&preview_file::task_id_) == c(&task::uuid_id_))),
      join<entity>(on(c(&task::entity_id_) == c(&entity::uuid_id_))),
      join<sequence>(on(c(&entity::parent_id_) == c(sequence->*&entity::uuid_id_))),
      // join<episode>(on(c(&entity::parent_id_) == c(episode->*&entity::uuid_id_))),
      where(
          c(sequence->*&entity::uuid_id_) == in_sequence_id &&
          (c(&preview_file::source_) == preview_file_source_enum::auto_light_generate ||
           c(&preview_file::source_) == preview_file_source_enum::vfx_review)
      ),
      order_by(&preview_file::created_at_).desc()
  );

  std::set<uuid> l_set;
  std::vector<get_sequence_shots_preview_t> l_result;

  for (auto&& [l_preview_file, l_entity_id, l_entity_name] : l_shots) {
    if (!l_set.contains(l_entity_id)) {
      l_set.emplace(l_entity_id);
      l_result.push_back(get_sequence_shots_preview_t{l_entity_id, l_preview_file, l_entity_name});
    }
  }

  std::sort(l_result.begin(), l_result.end());

  return l_result;
}
}  // namespace
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_sequences_create_review_playlists, post) {
  auto l_sql      = g_ctx().get<sqlite_database>();
  auto l_sequence = l_sql.get_by_uuid<entity>(sequence_id_);
  person_.check_in_project(l_sequence.project_id_);
  person_.check_not_outsourcer();
  auto l_playlist         = std::make_shared<playlist>();
  l_playlist->name_       = fmt::format("Review Playlist - {}", l_sequence.name_);
  l_playlist->project_id_ = l_sequence.project_id_;
  l_playlist->for_entity_ = "shot";
  co_await l_sql.install(l_playlist);
  auto l_playlist_shots = std::make_shared<std::vector<playlist_shot>>();
  for (auto&& l_preview_file : get_sequence_shots_preview(sequence_id_)) {
    l_playlist_shots->push_back(
        playlist_shot{
            .playlist_id_ = l_playlist->uuid_id_,
            .entity_id_   = l_preview_file.shot_id_,
            .preview_id_  = l_preview_file.preview_.uuid_id_,
            .order_index_ = static_cast<std::int32_t>(l_playlist_shots->size() * 100)
        }
    );
  }
  co_await l_sql.install_range(l_playlist_shots);
  co_return in_handle->make_msg(nlohmann::json{} = get_playlist_shot_entity(*l_playlist));
}
}  // namespace doodle::http