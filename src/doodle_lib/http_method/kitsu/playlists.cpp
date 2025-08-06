//
// Created by TD on 25-5-14.
//
#include <doodle_core/metadata/entity.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> playlists_entities_preview_files::get(
    session_data_ptr in_handle
) {
  auto l_sql     = g_ctx().get<sqlite_database>();
  auto l_entt_id = l_sql.get_by_uuid<entity>(id_);
  person_.is_project_access(l_entt_id.project_id_);
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
  person_.is_project_access(l_task.project_id_);
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
  l_prev->set_annotations(std::move(l_time_map | ranges::views::values | ranges::to_vector));
  co_await l_sql.install(l_prev);
  co_return in_handle->make_msg(nlohmann::json{} = *l_prev);
}

}  // namespace doodle::http