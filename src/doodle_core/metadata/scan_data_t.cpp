//
// Created by TD on 24-9-11.
//

#include "scan_data_t.h"

#include "sqlite_orm/sqlite_database.h"
#include <sqlite_orm/sqlite_orm.h>
#include <sqlite_orm/uuid_to_blob.h>
namespace doodle {

void scan_data_t::ue_path(const FSys::path& in_path) {
  BOOST_ASSERT(handle_);
  auto l_uuid = FSys::software_flag_file(in_path);
  if (handle_.any_of<additional_data>()) {
    handle_.patch<additional_data>().ue_path_ = in_path;
  } else {
    handle_.emplace<additional_data>().ue_path_ = in_path;
  }
  if (auto& l_s = handle_.registry()->storage<uuid>(detail::ue_path_id); l_s.contains(handle_)) {
    l_s.patch(handle_) = l_uuid;
  } else {
    l_s.emplace(handle_, l_uuid);
  }
}
std::tuple<uuid, FSys::path> scan_data_t::ue_path() const {
  BOOST_ASSERT(handle_);
  if (auto& l_s = handle_.registry()->storage<uuid>(detail::ue_path_id);
      l_s.contains(handle_) && handle_.all_of<additional_data>()) {
    return std::make_tuple(l_s.get(handle_), handle_.get<additional_data>().ue_path_);
  }
  return std::make_tuple(uuid{}, FSys::path{});
}

void scan_data_t::rig_path(const FSys::path& in_path) {
  BOOST_ASSERT(handle_);
  auto l_uuid = FSys::software_flag_file(in_path);
  if (handle_.any_of<additional_data>()) {
    handle_.patch<additional_data>().rig_path_ = in_path;
  } else {
    handle_.emplace<additional_data>().rig_path_ = in_path;
  }
  if (auto& l_s = handle_.registry()->storage<uuid>(detail::rig_path_id); l_s.contains(handle_)) {
    l_s.patch(handle_) = l_uuid;
  } else {
    l_s.emplace(handle_, l_uuid);
  }
}

std::tuple<uuid, FSys::path> scan_data_t::rig_path() const {
  BOOST_ASSERT(handle_);
  if (auto& l_s = handle_.registry()->storage<uuid>(detail::rig_path_id);
      l_s.contains(handle_) && handle_.all_of<additional_data>()) {
    return std::make_tuple(l_s.get(handle_), handle_.get<additional_data>().rig_path_);
  }
  return std::make_tuple(uuid{}, FSys::path{});
}

void scan_data_t::solve_path(const FSys::path& in_path) {
  BOOST_ASSERT(handle_);
  auto l_uuid = FSys::software_flag_file(in_path);
  if (handle_.any_of<additional_data>()) {
    handle_.patch<additional_data>().solve_path_ = in_path;
  } else {
    handle_.emplace<additional_data>().solve_path_ = in_path;
  }
  if (auto& l_s = handle_.registry()->storage<uuid>(detail::solve_path_id); l_s.contains(handle_)) {
    l_s.patch(handle_) = l_uuid;
  } else {
    l_s.emplace(handle_, l_uuid);
  }
}

std::tuple<uuid, FSys::path> scan_data_t::solve_path() const {
  BOOST_ASSERT(handle_);
  if (auto& l_s = handle_.registry()->storage<uuid>(detail::solve_path_id);
      l_s.contains(handle_) && handle_.all_of<additional_data>()) {
    return std::make_tuple(l_s.get(handle_), handle_.get<additional_data>().solve_path_);
  }
  return std::make_tuple(uuid{}, FSys::path{});
}

void scan_data_t::project(entt::entity in_project) {
  BOOST_ASSERT(handle_);
  if (auto& l_s = handle_.registry()->storage<entt::id_type>(detail::project_ref_id); l_s.contains(handle_)) {
    l_s.patch(handle_) = entt::to_integral(in_project);
  } else {
    l_s.emplace(handle_, entt::to_integral(in_project));
  }
}

entt::entity scan_data_t::project() const {
  BOOST_ASSERT(handle_);
  if (auto& l_s = handle_.registry()->storage<entt::id_type>(detail::project_ref_id);
      l_s.contains(handle_) && handle_.all_of<additional_data>()) {
    return entt::entity{l_s.get(handle_)};
  }
  return entt::null;
}

void scan_data_t::set_other(const additional_data2& in_other) {
  BOOST_ASSERT(handle_);
  if (handle_.any_of<additional_data2>()) {
    handle_.patch<additional_data2>() = in_other;
  } else {
    handle_.emplace<additional_data2>() = in_other;
  }
}

const scan_data_t::additional_data2& scan_data_t::get_other() const {
  BOOST_ASSERT(handle_);
  if (handle_.any_of<additional_data2>()) {
    return handle_.get<additional_data2>();
  } else {
    return handle_.emplace<additional_data2>();
  }
}

void scan_data_t::destroy() {
  BOOST_ASSERT(handle_);
  if (auto& l_s = handle_.registry()->storage<entt::id_type>(detail::sql_id);
      l_s.contains(handle_) && l_s.get(handle_) != 0)
    g_ctx().get<sqlite_database>().destroy<scan_data_t>(l_s.get(handle_));
  handle_.destroy();
}
void scan_data_t::seed_to_sql() {
  if (handle_) {
    database_t l_ret{};
    if (auto& l_s = handle_.registry()->storage<uuid>(detail::ue_path_id); l_s.contains(handle_)) {
      l_ret.ue_uuid_ = l_s.get(handle_);
    }
    if (auto& l_s = handle_.registry()->storage<uuid>(detail::rig_path_id); l_s.contains(handle_)) {
      l_ret.rig_uuid_ = l_s.get(handle_);
    }
    if (auto& l_s = handle_.registry()->storage<uuid>(detail::solve_path_id); l_s.contains(handle_)) {
      l_ret.solve_uuid_ = l_s.get(handle_);
    }
    if (auto& l_s = handle_.registry()->storage<entt::id_type>(detail::project_ref_id); l_s.contains(handle_)) {
      entt::entity l_prj_ett{l_s.get(handle_)};
      if (auto& l_prj_s = handle_.registry()->storage<uuid>(detail::project_id);
          l_prj_ett != entt::null && l_prj_s.contains(l_prj_ett)) {
        l_ret.project_ = l_prj_s.get(l_prj_ett);
      }
    }
    auto& l_a         = handle_.get<additional_data>();
    auto& l_a2        = handle_.get<additional_data2>();
    l_ret.ue_path_    = l_a.ue_path_.generic_string();
    l_ret.rig_path_   = l_a.rig_path_.generic_string();
    l_ret.solve_path_ = l_a.solve_path_.generic_string();
    l_ret.num_        = l_a2.num_;
    l_ret.name_       = l_a2.name_;
    l_ret.version_    = l_a2.version_;

    if (auto& l_s = handle_.registry()->storage<std::int32_t>(detail::sql_id); l_s.contains(handle_)) {
      l_ret.id_ = l_s.get(handle_);
    }
    g_ctx().get<sqlite_database>()(std::move(l_ret));
  }
}

std::vector<entt::entity> scan_data_t::load_from_sql(
    entt::registry& in_registry, const std::vector<database_t>& in_data
) {
  std::vector<entt::entity> l_create{};
  if (in_data.empty()) return l_create;

  in_registry.create(l_create.begin(), l_create.end());

  {
    auto& l_s = in_registry.storage<uuid>(detail::ue_path_id);
    std::vector<uuid> l_vec =
        in_data | ranges::views::transform([](const auto& in_db) { return in_db.ue_uuid_; }) | ranges::to_vector;
    l_s.insert(l_create.begin(), l_create.end(), l_vec.begin());
  }
  {
    auto& l_s = in_registry.storage<uuid>(detail::rig_path_id);
    std::vector<uuid> l_vec =
        in_data | ranges::views::transform([](const auto& in_db) { return in_db.rig_uuid_; }) | ranges::to_vector;
    l_s.insert(l_create.begin(), l_create.end(), l_vec.begin());
  }
  {
    auto& l_s = in_registry.storage<uuid>(detail::solve_path_id);
    std::vector<uuid> l_vec =
        in_data | ranges::views::transform([](const auto& in_db) { return in_db.solve_uuid_; }) | ranges::to_vector;
    l_s.insert(l_create.begin(), l_create.end(), l_vec.begin());
  }
  {
    auto& l_s = in_registry.storage<entt::id_type>(detail::sql_id);
    std::vector<entt::id_type> l_vec =
        in_data | ranges::views::transform([](const auto& in_db) -> entt::id_type { return in_db.id_; }) |
        ranges::to_vector;
    l_s.insert(l_create.begin(), l_create.end(), l_vec.begin());
  }
  {
    std::map<uuid, entt::entity> l_prj_map{};
    for (auto&& [l_e, l_id] : entt::view<entt::get_t<uuid>>{in_registry.storage<uuid>(detail::project_id)}.each()) {
      l_prj_map.emplace(l_id, l_e);
    }
    auto& l_s                        = in_registry.storage<entt::id_type>(detail::project_ref_id);

    std::vector<entt::id_type> l_vec = in_data |
                                       ranges::views::transform([&l_prj_map](const database_t& in_db) -> entt::id_type {
                                         if (auto it = l_prj_map.find(in_db.project_); it != l_prj_map.end()) {
                                           return entt::to_integral(it->second);
                                         } else {
                                           return entt::to_integral(entt::entity{entt::null});
                                         }
                                       }) |
                                       ranges::to_vector;
    l_s.insert(l_create.begin(), l_create.end(), l_vec.begin());
  }
  std::vector<additional_data> l_vec = in_data | ranges::views::transform([](const database_t& in_db) {
                                         return additional_data{in_db.ue_path_, in_db.rig_path_, in_db.solve_path_};
                                       }) |
                                       ranges::to_vector;
  std::vector l_vec2 = in_data | ranges::views::transform([](const database_t& in_db) {
                         return additional_data2{in_db.num_, in_db.name_, in_db.version_};
                       }) |
                       ranges::to_vector;
  in_registry.insert<additional_data>(l_create.begin(), l_create.end(), l_vec.begin());
  in_registry.insert<additional_data2>(l_create.begin(), l_create.end(), l_vec2.begin());
  return l_create;
}

}  // namespace doodle