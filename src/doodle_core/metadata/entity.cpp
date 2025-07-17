//
// Created by TD on 25-7-17.
//
#include "entity.h"

#include <doodle_core/metadata/entity_type.h>

#include "sqlite_orm/sqlite_database.h"
namespace doodle {

std::tuple<std::string, uuid> entity::get_full_name() const {
  std::string l_name_{};
  uuid l_ep_id{};
  if (entity_type_id_ == asset_type::get_shot_id()) {
    auto l_seq = g_ctx().get<sqlite_database>().get_by_uuid<entity>(parent_id_);
    if (l_seq.parent_id_.is_nil())
      l_name_ = fmt::format("{}/{}", l_seq.name_, name_);
    else {
      auto l_ep = g_ctx().get<sqlite_database>().get_by_uuid<entity>(l_seq.parent_id_);
      l_name_   = fmt::format("{}/{}/{}", l_ep.name_, l_seq.name_, name_);
      l_ep_id   = l_ep.uuid_id_;
    }
  } else if (entity_type_id_ == asset_type::get_episode_id()) {
    l_name_ = name_;
  } else if (entity_type_id_ == asset_type::get_sequence_id()) {
    l_name_ = name_;
    if (!parent_id_.is_nil()) {
      auto l_ep = g_ctx().get<sqlite_database>().get_by_uuid<entity>(parent_id_);
      l_name_   = fmt::format("{}/{}", l_ep.name_, name_);
      l_ep_id   = l_ep.uuid_id_;
    }
  } else {
    auto l_ass_type = g_ctx().get<sqlite_database>().get_by_uuid<asset_type>(entity_type_id_);
    l_name_         = fmt::format("{}/{}", l_ass_type.name_, name_);
    l_ep_id         = parent_id_;
  }
  return {l_name_, l_ep_id};
}
}  // namespace doodle