//
// Created by td_main on 2023/11/3.
//

#include "sqlite_snapshot.h"

#include <doodle_core/sqlite_orm/sqlite_base.h>
namespace doodle::snapshot {

entt::meta_any begin_save_entt(entt::entity&, const conn_ptr& in_conn) { return {}; }
void save_entt(entt::entity&, entt::entity& in_entity, entt::meta_any& in_pre, const conn_ptr& in_conn) {}

entt::meta_any begin_load_entt(entt::entity&, const conn_ptr& in_conn) { return {}; }
void load_entt(entt::entity&, entt::meta_any& in_pre, const conn_ptr& in_conn) {}

std::underlying_type_t<entt::entity> get_size_entt(entt::entity&, const conn_ptr& in_conn) { return {}; }

void sqlite_snapshot::init_base_meta() {
  entt::meta<entt::entity>()
      .func<&begin_save_entt>("begin_save"_hs)
      .func<&save_entt>("save"_hs)
      .func<&begin_load_entt>("begin_load"_hs)
      .func<&load_entt>("load"_hs)
      .func<&get_size_entt>("get_size"_hs);
}
}  // namespace doodle::snapshot