#include "entity.h"

#include <doodle_core/configure/static_value.h>
#include <doodle_core/metadata/entity_type.h>

#include <doodle_lib/sqlite_orm/sqlite_database.h>

namespace doodle::entity_ns {
std::tuple<std::string, uuid> get_full_name(const entity& in_entity) {
  std::string l_name_{};
  uuid l_ep_id{};
  if (in_entity.entity_type_id_ == asset_type::get_shot_id()) {
    auto l_seq = get_sqlite_database().get_by_uuid<entity>(in_entity.parent_id_);
    if (l_seq.parent_id_.is_nil())
      l_name_ = fmt::format("{}/{}", l_seq.name_, in_entity.name_);
    else {
      auto l_ep = get_sqlite_database().get_by_uuid<entity>(l_seq.parent_id_);
      l_name_   = fmt::format("{}/{}/{}", l_ep.name_, l_seq.name_, in_entity.name_);
      l_ep_id   = l_ep.uuid_id_;
    }
  } else if (in_entity.entity_type_id_ == asset_type::get_episode_id()) {
    l_name_ = in_entity.name_;
  } else if (in_entity.entity_type_id_ == asset_type::get_sequence_id()) {
    l_name_ = in_entity.name_;
    if (!in_entity.parent_id_.is_nil()) {
      auto l_ep = get_sqlite_database().get_by_uuid<entity>(in_entity.parent_id_);
      l_name_   = fmt::format("{}/{}", l_ep.name_, in_entity.name_);
      l_ep_id   = l_ep.uuid_id_;
    }
  } else {
    auto l_ass_type = get_sqlite_database().get_by_uuid<asset_type>(in_entity.entity_type_id_);
    l_name_         = fmt::format("{}/{}", l_ass_type.name_, in_entity.name_);
    l_ep_id         = in_entity.parent_id_;
  }
  return {l_name_, l_ep_id};
}
}  // namespace doodle::entity_ns