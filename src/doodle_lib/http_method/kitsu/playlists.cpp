//
// Created by TD on 25-5-14.
//
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/playlist.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> playlists_entities_preview_files::get(
    session_data_ptr in_handle
) {
  auto l_sql     = g_ctx().get<sqlite_database>();
  auto l_entt_id = l_sql.get_by_uuid<entity>(id_);
  person_.check_project_access(l_entt_id.project_id_);
  nlohmann::json l_json{};
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
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_prev = std::make_shared<preview_file>(l_sql.get_by_uuid<preview_file>(preview_file_id_));
  auto l_task = l_sql.get_by_uuid<task>(l_prev->task_id_);
  person_.check_project_access(l_task.project_id_);
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
  l_prev->updated_at_ = chrono::system_clock::now();
  co_await l_sql.install(l_prev);
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
      : id_(in_entt.uuid_id_), name_(in_entt.name_), preview_file_task_id_(in_task_id), preview_file_previews_(in_v) {
    if (!in_task_type_id.is_nil() && preview_file_previews_.map_.contains(in_task_type_id) &&
        !preview_file_previews_.map_.at(in_task_type_id).empty()) {
      auto&& l_pf               = preview_file_previews_.map_.at(in_task_type_id).front();
      preview_file_id_          = l_pf.uuid_id_;
      preview_file_extension_   = l_pf.extension_;
      preview_file_width_       = l_pf.width_;
      preview_file_height_      = l_pf.height_;
      preview_file_duration_    = l_pf.duration_;
      preview_file_revision_    = l_pf.revision_;
      preview_file_status_      = l_pf.status_;
      preview_file_annotations_ = l_pf.annotations_;
    }
  }

  struct preview_files_for_entity_map_t {
    std::map<uuid, std::vector<preview_files_for_entity_t>> map_;
    // to json
    friend void to_json(nlohmann::json& j, const preview_files_for_entity_map_t& p) {
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
  decltype(preview_file::annotations_) preview_file_annotations_;
  preview_files_for_entity_map_t preview_file_previews_;

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
    j["preview_file_annotations"] = p.preview_file_annotations_;
    j["preview_file_previews"]    = p.preview_file_previews_;

    j["preview_files"]            = p.preview_file_previews_;
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
    return lhs.sequence_name_ < rhs.sequence_name_;
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
  person_.check_project_access(project_id_);
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
  person_.check_project_access(project_id_);
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
  } else {
    l_order.push_back(order_by(&playlist::updated_at_).desc());
  }
  auto l_playlists = l_sql.impl_->storage_any_.get_all<playlist>(
      where(
          c(&playlist::project_id_) == project_id_ &&
          (l_task_type_id.is_nil() || c(&playlist::task_type_id_) == l_task_type_id)
      ),
      limit(l_offset, 20), l_order
  );
  co_return in_handle->make_msg(nlohmann::json{} = l_playlists);
}

}  // namespace doodle::http