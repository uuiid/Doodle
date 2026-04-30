#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/person.h"
#include "doodle_core/metadata/seedance2/assets_entity_item.h"
#include "doodle_core/metadata/seedance2/group.h"
#include "doodle_core/metadata/seedance2/task.h"
#include <doodle_core/metadata/ai_image_metadata.h>
#include <doodle_core/metadata/asset_instance.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/playlist.h>
#include <doodle_core/metadata/preview_file.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/project_status.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/studio.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_xlsx_task_info.h>
#include <doodle_core/metadata/working_file.h>

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>
#include <doodle_lib/sqlite_orm/sqlite_select_data.h>
#include <doodle_lib/sqlite_orm/sqlite_upgrade.h>

#include "sqlite_orm/detail/dynamic_where.h"
#include <spdlog/spdlog.h>
#include <sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>

namespace doodle::sqlite_select {

std::vector<ai_studio_and_link_t> ai_studio_and_link_t_get_all() {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_ai_studios = l_sql.impl_->storage_any_.select(
      columns(object<ai_studio>(), object<ai_studio_person_role_link>()),
      left_join<ai_studio_person_role_link>(
          on(c(&ai_studio_person_role_link::ai_studio_id_) == c(&ai_studio::uuid_id_))
      )
  );
  std::vector<ai_studio_and_link_t> l_result;
  std::map<uuid, std::size_t> l_map{};
  for (auto&& [ai_studio, link] : l_ai_studios) {
    if (!l_map.contains(ai_studio.uuid_id_)) {
      l_result.emplace_back(ai_studio);
      l_map[ai_studio.uuid_id_] = l_result.size() - 1;
    }
    if (!link.ai_studio_id_.is_nil()) l_result[l_map[ai_studio.uuid_id_]].link_.push_back(link);
  }
  return l_result;
}

std::string get_rig_person_last_name_for_entity(const uuid& in_entity_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;

  auto l_task = l_sql.impl_->storage_any_.select(
      &task::uuid_id_, from<task>(),
      where(c(&task::entity_id_) == in_entity_id && c(&task::task_type_id_) == task_type::get_binding_id()), limit(1)
  );
  std::string l_user_abbreviation{};
  if (!l_task.empty()) {
    auto l_user = l_sql.impl_->storage_any_.select(
        &person::last_name_, from<person>(),
        where(
            in(&person::uuid_id_,
               select(&assignees_table::person_id_, where(c(&assignees_table::task_id_) == l_task.front())))
        ),
        limit(1)
    );

    if (!l_user.empty()) return l_user.front();
  }
  return {};
}
uuid get_ai_studio_uuid_for_person(const uuid& in_person_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_vec = l_sql.impl_->storage_any_.get_all<ai_studio_person_role_link>(
      where(c(&ai_studio_person_role_link::person_id_) == in_person_id)
  );
  return l_vec.empty() ? uuid{} : l_vec.front().ai_studio_id_;
}
// 获取任务分配的人(连接表)
std::optional<assignees_table> get_task_assignees_for_task_and_person(uuid in_task_id, uuid in_person_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_ret = l_sql.impl_->storage_any_.get_all<assignees_table>(
      where(c(&assignees_table::task_id_) == in_task_id && c(&assignees_table::person_id_) == in_person_id)
  );
  return !l_ret.empty() ? std::optional{l_ret.front()} : std::optional<assignees_table>{std::nullopt};
}
std::vector<std::int64_t> get_task_assignees_ids_for_task(uuid in_task_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_ret =
      l_sql.impl_->storage_any_.select(&assignees_table::id_, where(c(&assignees_table::task_id_) == in_task_id));
  return l_ret;
}
std::vector<sd2::task> get_sd2_tasks_for_ai_studio(const uuid& in_ai_studio_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_vec = l_sql.impl_->storage_any_.get_all<sd2::task>(
      where(c(&sd2::task::ai_studio_id_) == in_ai_studio_id) && !c(&sd2::task::archived_)
  );
  return l_vec;
}

std::vector<sd2::task> get_sd2_tasks_for_person(const uuid& in_person_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_vec = l_sql.impl_->storage_any_.get_all<sd2::task>(
      where(c(&sd2::task::user_id_) == in_person_id) && !c(&sd2::task::archived_)
  );
  return l_vec;
}
std::vector<sd2::assets_group> get_sd2_assets_group_for_ai_studio(const uuid& in_ai_studio_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_vec = l_sql.impl_->storage_any_.get_all<sd2::assets_group>(
      where(c(&sd2::assets_group::ai_studio_id_) == in_ai_studio_id)
  );
  return l_vec;
}

std::size_t get_sd2_assets_count_for_assets_group(const uuid& in_assets_group_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_size = l_sql.impl_->storage_any_.count<sd2::assets_entity>(
      where(c(&sd2::assets_entity::group_id_) == in_assets_group_id)
  );
  return l_size;
}
std::vector<assets_entity_and_item> get_assets_entity_and_item_all_for_person_and_ai_studio(
    const uuid& in_group_id, const uuid& in_ai_studio_id
) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_entities = l_sql.impl_->storage_any_.select(
      columns(object<sd2::assets_entity>(), object<sd2::assets_entity_item>()),
      left_join<sd2::assets_entity_item>(
          on(c(&sd2::assets_entity_item::parent_id_) == c(&sd2::assets_entity::uuid_id_))
      ),
      where(
          c(&sd2::assets_entity::group_id_) == in_group_id && c(&sd2::assets_entity::ai_studio_id_) == in_ai_studio_id
      )
  );
  std::vector<assets_entity_and_item> l_result{};
  std::map<uuid, std::size_t> l_map{};
  for (auto&& [entity, item] : l_entities) {
    if (!l_map.contains(entity.uuid_id_)) {
      l_result.emplace_back(entity);
      l_map[entity.uuid_id_] = l_result.size() - 1;
    }
    if (!item.uuid_id_.is_nil()) l_result[l_map[entity.uuid_id_]].items_.push_back(item);
  }
  return l_result;
}
std::vector<sd2::assets_entity> search_sd2_assets_entity_for_ai_studio(
    const uuid& in_ai_studio_id, const std::string& keyword
) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_vec = l_sql.impl_->storage_any_.get_all<sd2::assets_entity>(
      where(like(&sd2::assets_entity::name_, keyword) && c(&sd2::assets_entity::ai_studio_id_) == in_ai_studio_id)
  );
  return l_vec;
}
bool entity_has_simulation_asset(const uuid& in_entity_id) {
  using namespace sqlite_orm;
  return get_sqlite_database().impl_->storage_any_.count<task>(
             where(c(&task::entity_id_) == in_entity_id && c(&task::task_type_id_) == task_type::get_simulation_id())
         ) > 0;
}

std::vector<std::tuple<entity, entity_asset_extend>> get_working_files_for_entity(
    const uuid& in_project_id, const uuid& in_shot_id, const uuid& in_sequence_id
) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;

  constexpr auto shot     = "shot"_alias.for_<entity>();
  constexpr auto sequence = "sequence"_alias.for_<entity>();
  auto l_assets           = l_sql.impl_->storage_any_.select(
      columns(object<entity>(true), object<entity_asset_extend>(true)), from<entity>(),
      left_outer_join<entity_asset_extend>(on(c(&entity_asset_extend::entity_id_) == c(&entity::uuid_id_))),
      where(in(
          &entity::uuid_id_, select(
                                 &entity_link::entity_out_id_, from<entity_link>(),
                                 join<shot>(on(c(&entity_link::entity_in_id_) == c(shot->*&entity::uuid_id_))),
                                 join<sequence>(on(c(shot->*&entity::parent_id_) == c(sequence->*&entity::uuid_id_))),
                                 where(
                                     (in_shot_id.is_nil() || c(shot->*&entity::uuid_id_) == in_shot_id) &&
                                     (in_sequence_id.is_nil() || c(sequence->*&entity::uuid_id_) == in_sequence_id)
                                 )
                             )
      ))
  );

  return l_assets;
}
std::vector<std::tuple<entity, entity_asset_extend>> get_working_files_for_entity(const uuid& in_entity_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_r = l_sql.impl_->storage_any_.select(
      columns(object<entity>(true), object<entity_asset_extend>(true)), from<entity>(),
      left_outer_join<entity_asset_extend>(on(c(&entity_asset_extend::entity_id_) == c(&entity::uuid_id_))),
      where(c(&entity::uuid_id_) == in_entity_id)
  );
  return l_r;
}
std::vector<std::tuple<entity, entity_asset_extend>> get_working_files_for_entity(
    const std::vector<uuid>& in_entity_ids
) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_r = l_sql.impl_->storage_any_.select(
      columns(object<entity>(true), object<entity_asset_extend>(true)), from<entity>(),
      left_outer_join<entity_asset_extend>(on(c(&entity_asset_extend::entity_id_) == c(&entity::uuid_id_))),
      where(in(&entity::uuid_id_, in_entity_ids))
  );
  return l_r;
}

std::vector<std::tuple<entity_link, std::string, uuid, uuid, std::string>>
get_sequence_casting_for_project_and_person_and_sequence(
    const uuid& in_project_id, const person& in_person, const uuid& in_sequence_id, const std::vector<uuid>& in_shot_ids
) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  constexpr auto shot     = "shot"_alias.for_<entity>();
  constexpr auto sequence = "sequence"_alias.for_<entity>();
  auto l_outsource_select = select(
      &outsource_studio_authorization::entity_id_,
      where(c(&outsource_studio_authorization::studio_id_) == in_person.studio_id_)
  );

  auto l_r = l_sql.impl_->storage_any_.select(
      columns(
          object<entity_link>(true), &entity::name_, &entity::preview_file_id_, &entity::project_id_, &asset_type::name_
      ),
      from<entity_link>(), join<shot>(on(c(&entity_link::entity_in_id_) == c(shot->*&entity::uuid_id_))),
      join<sequence>(on(c(shot->*&entity::parent_id_) == c(sequence->*&entity::uuid_id_))),
      join<entity>(on(c(&entity_link::entity_out_id_) == c(&entity::uuid_id_))),
      join<asset_type>(on(c(&entity::entity_type_id_) == c(&asset_type::uuid_id_))),
      where(
          c(&entity::canceled_) != true && (in_project_id.is_nil() || c(&entity::project_id_) == in_project_id) &&
          (in_sequence_id.is_nil() || c(sequence->*&entity::uuid_id_) == in_sequence_id) &&
          (in_shot_ids.empty() || in(shot->*&entity::uuid_id_, in_shot_ids)) &&                                //
          (in_person.role_ != person_role_type::outsource || (                                                 //
                                                                 in(&entity::uuid_id_, l_outsource_select) ||  //
                                                                 in(sequence->*&entity::uuid_id_, l_outsource_select)
                                                             ))
      ),
      multi_order_by(
          order_by(sequence->*&entity::name_), order_by(shot->*&entity::name_), order_by(&asset_type::name_),
          order_by(&entity::name_)
      )
  );
  return l_r;
}
std::vector<std::tuple<entity_link, std::string, uuid, uuid, std::string>>
get_sequence_casting_for_project_and_asset_type(const uuid& in_project_id, const uuid& in_asset_type_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  constexpr auto asset = "asset"_alias.for_<entity>();
  auto l_r             = l_sql.impl_->storage_any_.select(

      columns(
          object<entity_link>(true), &entity::name_, &entity::preview_file_id_, &entity::project_id_, &asset_type::name_
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

  );
  return l_r;
}

std::vector<entity_link> get_entity_link_by_entity_id(const uuid& in_entity_id) {
  auto l_sql = get_sqlite_database();

  using namespace sqlite_orm;
  auto l_ret = l_sql.impl_->storage_any_.get_all<entity_link>(where(c(&entity_link::entity_in_id_) == in_entity_id));

  return l_ret;
}
std::vector<entity_link> get_entity_link_by_entity_id(const std::vector<uuid>& in_entity_id) {
  auto l_sql = get_sqlite_database();

  using namespace sqlite_orm;
  auto l_ret = l_sql.impl_->storage_any_.get_all<entity_link>(where(in(&entity_link::entity_in_id_, in_entity_id)));

  return l_ret;
}
std::vector<std::tuple<entity_link, std::string, std::string, uuid, uuid, uuid, uuid>> get_sequence_casting_for_entity(
    const uuid& in_entity_id
) {
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
  return l_r;
}
std::vector<std::tuple<entity, outsource_studio_authorization, entity_asset_extend, entity_shot_extend>>
get_entity_and_outsource_studio_authorization_by_project_id(const uuid& in_project_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_rows = l_sql.impl_->storage_any_.iterate(select(
      columns(
          object<entity>(true), object<outsource_studio_authorization>(true), object<entity_asset_extend>(true),
          object<entity_shot_extend>(true)
      ),
      from<entity>(),
      left_outer_join<outsource_studio_authorization>(
          on(c(&outsource_studio_authorization::entity_id_) == c(&entity::uuid_id_))
      ),
      left_outer_join<entity_asset_extend>(on(c(&entity_asset_extend::entity_id_) == c(&entity::uuid_id_))),
      left_outer_join<entity_shot_extend>(on(c(&entity_shot_extend::entity_id_) == c(&entity::uuid_id_))),
      where(c(&entity::project_id_) == in_project_id), order_by(&entity::name_)
  ));
  std::vector<std::tuple<entity, outsource_studio_authorization, entity_asset_extend, entity_shot_extend>> l_result{};
  l_result.reserve(l_sql.get_project_entity_count(in_project_id));
  for (auto&& [l_entity, l_authorization, l_entity_asset_extend, l_entity_shot_extend] : l_rows)
    l_result.emplace_back(l_entity, l_authorization, l_entity_asset_extend, l_entity_shot_extend);

  return l_result;
}
std::vector<std::tuple<entity, entity_asset_extend>> get_entity_and_entity_asset_extend_by_shot_id(
    const uuid& in_shot_id
) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  constexpr auto shot     = "shot"_alias.for_<entity>();
  constexpr auto sequence = "sequence"_alias.for_<entity>();
  auto l_assets           = l_sql.impl_->storage_any_.select(
      columns(object<entity>(true), object<entity_asset_extend>(true)), from<entity>(),
      left_outer_join<entity_asset_extend>(on(c(&entity_asset_extend::entity_id_) == c(&entity::uuid_id_))),
      where(
          in(&entity::uuid_id_,
                       select(
                 &entity_link::entity_out_id_, from<entity_link>(),
                 join<shot>(on(c(&entity_link::entity_in_id_) == c(shot->*&entity::uuid_id_))),
                 join<sequence>(on(c(shot->*&entity::parent_id_) == c(sequence->*&entity::uuid_id_))),
                 where(c(shot->*&entity::uuid_id_) == in_shot_id)
             )) &&
          !c(&entity::canceled_)
      )
  );
  return l_assets;
}
std::vector<preview_file> get_preview_files_by_entity_id(const uuid& in_entity_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_ret = l_sql.impl_->storage_any_.get_all<preview_file>(
      join<task>(on(c(&preview_file::task_id_) == c(&task::uuid_id_))),
      join<task_type>(on(c(&task::task_type_id_) == c(&task_type::uuid_id_))),
      where(c(&task::entity_id_) == in_entity_id),
      multi_order_by(
          order_by(&task_type::name_), order_by(&preview_file::revision_).desc(), order_by(&preview_file::position_)
      )
  );
  return l_ret;
}
std::optional<entity_asset_extend> get_entity_asset_extend_by_entity_id(const uuid& in_entity_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_ret = l_sql.impl_->storage_any_.get_all<entity_asset_extend>(
      where(c(&entity_asset_extend::entity_id_) == in_entity_id)
  );
  return !l_ret.empty() ? std::optional{l_ret.front()} : std::optional<entity_asset_extend>{std::nullopt};
}
std::optional<entity_shot_extend> get_entity_shot_extend_by_entity_id(const uuid& in_entity_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_ret =
      l_sql.impl_->storage_any_.get_all<entity_shot_extend>(where(c(&entity_shot_extend::entity_id_) == in_entity_id));
  return !l_ret.empty() ? std::optional{l_ret.front()} : std::optional<entity_shot_extend>{std::nullopt};
}
std::optional<computer> get_entity_computer_by_hardware_id(const uuid& in_hardware_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_ret = l_sql.impl_->storage_any_.get_all<computer>(where(c(&computer::hardware_id_) == in_hardware_id));
  return !l_ret.empty() ? std::optional{l_ret.front()} : std::optional<computer>{std::nullopt};
}
std::vector<uuid> get_comment_object_ids_by_comment_id(const uuid& in_comment_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_ret = l_sql.impl_->storage_any_.select(&comment::object_id_, where(c(&comment::uuid_id_) == in_comment_id));
  return l_ret;
}
std::vector<uuid> get_task_project_ids_by_task_id(const uuid& in_task_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_ret = l_sql.impl_->storage_any_.select(&task::project_id_, where(c(&task::uuid_id_) == in_task_id));
  return l_ret;
}
std::vector<std::int32_t> get_comment_acknowledgement_ids_by_comment_id_and_person_id(
    const uuid& in_comment_id, const uuid& in_person_id
) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_ret = l_sql.impl_->storage_any_.select(
      &comment_acknoledgments::id_, where(
                                        c(&comment_acknoledgments::comment_id_) == in_comment_id &&
                                        c(&comment_acknoledgments::person_id_) == in_person_id
                                    )
  );
  return l_ret;
}
std::vector<attachment_file> get_attachment_files_by_comment_id(const uuid& in_comment_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_ret =
      l_sql.impl_->storage_any_.get_all<attachment_file>(where(c(&attachment_file::comment_id_) == in_comment_id));
  return l_ret;
}
std::vector<playlist> get_playlist_by_task_type_and_project::operator()() const {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_order = dynamic_order_by(l_sql.impl_->storage_any_);
  switch (order_by_) {
    case order_by_enum::create_at:
      l_order.push_back(order_by(&playlist::created_at_).desc());
      break;
    case order_by_enum::name:
      l_order.push_back(order_by(&playlist::name_));
      break;
    case order_by_enum::update_at:
      l_order.push_back(order_by(&playlist::updated_at_).desc());
      break;
    default:
      l_order.push_back(order_by(&playlist::updated_at_).desc());
  }
  std::int32_t l_offset = (page_ - 1) * 20;

  auto l_playlists      = l_sql.impl_->storage_any_.get_all<playlist>(
      where(
          c(&playlist::project_id_) == project_id_ &&
          (task_type_id_.is_nil() || c(&playlist::task_type_id_) == task_type_id_)
      ),
      l_order, limit(l_offset, 20)
  );
  return l_playlists;
}
std::vector<std::tuple<preview_file, uuid, uuid>> get_preview_files_and_task_type_id_and_task_entity_id_in_entity_ids(
    const std::vector<uuid>& in_entity_ids
) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_result = l_sql.impl_->storage_any_.select(
      columns(object<preview_file>(true), &task::task_type_id_, &task::entity_id_),
      join<task>(on(c(&task::uuid_id_) == c(&preview_file::task_id_))),
      join<task_type>(on(c(&task::task_type_id_) == c(&task_type::uuid_id_))),
      where(in(&task::entity_id_, in_entity_ids)),
      multi_order_by(
          order_by(&task_type::priority_).desc(), order_by(&task_type::name_),
          order_by(&preview_file::revision_).desc(), order_by(&preview_file::position_),
          order_by(&preview_file::created_at_)
      )
  );
  return l_result;
}
std::size_t count_playlist_shots_by_playlist_shot_id(const uuid& in_playlist_shot_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_count =
      l_sql.impl_->storage_any_.count<playlist_shot>(where(c(&playlist_shot::playlist_id_) == in_playlist_shot_id));
  return l_count;
}
std::vector<std::tuple<preview_file, uuid, std::string>> get_preview_files_and_entity_id_and_entity_name_by_sequence_id(
    const uuid& in_sequence_id
) {
  auto l_sql = get_sqlite_database();
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
  return l_shots;
}

std::vector<std::tuple<notification, entity, comment, uuid, std::string, uuid, uuid>>
get_notifications_and_entity_and_comment_and_project_id_and_project_name_and_task_id_and_task_name_by_person_id(
    const uuid& in_person_id, const std::optional<chrono::system_zoned_time>& in_after,
    const std::optional<chrono::system_zoned_time>& in_before, const uuid& in_task_type_id,
    const uuid& in_task_status_id, const std::optional<notification_type>& in_notification_type,
    const std::optional<bool>& in_read
) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_row = l_sql.impl_->storage_any_.select(
      columns(
          object<notification>(true), object<entity>(true), object<comment>(true), &project::uuid_id_, &project::name_,
          &task::task_type_id_, &subscription::uuid_id_
      ),
      from<notification>(),  //
      join<task>(on(c(&notification::task_id_) == c(&task::uuid_id_))),
      join<project>(on(c(&task::project_id_) == c(&project::uuid_id_))),
      left_outer_join<entity>(on(c(&task::entity_id_) == c(&entity::uuid_id_))),
      left_outer_join<comment>(on(c(&notification::comment_id_) == c(&comment::uuid_id_))),
      left_outer_join<subscription>(
          on(c(&subscription::task_id_) == c(&task::uuid_id_) && c(&subscription::person_id_) == in_person_id)
      ),

      where(
          c(&notification::person_id_) == in_person_id &&
          (!in_after.has_value() || c(&notification::created_at_) > in_after.value_or(chrono::system_zoned_time{})) &&
          (!in_before.has_value() || c(&notification::created_at_) < in_before.value_or(chrono::system_zoned_time{})) &&
          (in_task_type_id.is_nil() || c(&task::task_type_id_) == in_task_type_id) &&
          (in_task_status_id.is_nil() || c(&comment::task_status_id_) == in_task_status_id) &&
          (!in_notification_type.has_value() ||
           c(&notification::type_) == in_notification_type.value_or(notification_type::comment)) &&
          (!in_read.has_value() || c(&notification::read_) == in_read.value_or(true))
      )
  );
  return l_row;
}

std::vector<uuid> get_comment_mentions_person_ids_by_comment_id(const uuid& in_comment_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_row = l_sql.impl_->storage_any_.select(
      &comment_mentions::person_id_, where(c(&comment_mentions::comment_id_) == in_comment_id)
  );
  return l_row;
}
std::vector<uuid> get_comment_department_mentions_department_ids_by_comment_id(const uuid& in_comment_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_row = l_sql.impl_->storage_any_.select(
      &comment_department_mentions::department_id_, where(c(&comment_department_mentions::comment_id_) == in_comment_id)
  );
  return l_row;
}

std::optional<preview_file> get_preview_files_by_entity_id_and_simulation_task_type_and_lighting_animation(
    const uuid& in_entity_id
) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_preview_files = l_sql.impl_->storage_any_.get_all<preview_file>(
      join<task>(on(c(&preview_file::task_id_) == c(&task::uuid_id_))),
      where(
          c(&task::entity_id_) == in_entity_id &&
          in(&task::task_type_id_,
             {task_type::get_simulation_task_id(), task_type::get_lighting_id(), task_type::get_animation_id()}) &&
          c(&preview_file::source_) == preview_file_source_enum::auto_light_generate
      ),
      order_by(&preview_file::created_at_).desc(), limit(1)
  );
  return !l_preview_files.empty() ? std::optional{l_preview_files.front()} : std::optional<preview_file>{std::nullopt};
}
std::vector<attachment_file> get_attachment_files_by_comment_id_and_task_id(const uuid& in_task_id) {
  auto l_sql = get_sqlite_database();

  using namespace sqlite_orm;
  auto l_attachment_files = l_sql.impl_->storage_any_.get_all<attachment_file>(where(
      in(&attachment_file::comment_id_,
         select(&comment::uuid_id_, from<comment>(), where(c(&comment::object_id_) == in_task_id)))
  ));
  return l_attachment_files;
}
std::vector<std::tuple<uuid, std::string>> get_project_ids_and_names() {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_projects = l_sql.impl_->storage_any_.select(columns(&project::uuid_id_, &project::name_));
  return l_projects;
}
std::vector<std::tuple<
    uuid,                 // task::uuid_id_
    std::string,          // task::name_
    uuid,                 // task::last_preview_file_id_
    uuid,                 // entity::uuid_id_
    std::string,          // entity::name_
    uuid,                 // task_type::uuid_id_
    entity_asset_extend,  // entity_asset_extend
    uuid,                 // project::uuid_id_
    std::string           // project::name_
    >>
get_tasks_and_entities_and_entity_asset_extend_and_project_by_task_ids(const std::vector<uuid>& in_task_ids) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_row = l_sql.impl_->storage_any_.select(
      columns(
          &task::uuid_id_, &task::name_, &task::last_preview_file_id_,

          &entity::uuid_id_, &entity::name_, &task_type::uuid_id_,

          object<entity_asset_extend>(true),

          &project::uuid_id_, &project::name_

      ),
      from<task>(), join<entity>(on(c(&task::entity_id_) == c(&entity::uuid_id_))),
      join<task_type>(on(c(&task_type::uuid_id_) == c(&task::task_type_id_))),
      left_outer_join<entity_asset_extend>(on(&entity_asset_extend::entity_id_) == c(&entity::uuid_id_)),
      join<project>(on(c(&project::uuid_id_) == c(&task::project_id_))), where(in(&task::uuid_id_, in_task_ids))
  );
  return l_row;
}
std::vector<std::int32_t> get_work_xlsx_task_info_helper_database_t_id_by_person_id_and_year_month(
    const uuid& in_person_id, const chrono::local_days& in_year_month
) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_ids = l_sql.impl_->storage_any_.select(
      &work_xlsx_task_info_helper::database_t::id_,
      where(
          c(&work_xlsx_task_info_helper::database_t::person_id_) == in_person_id &&
          c(&work_xlsx_task_info_helper::database_t::year_month_) == in_year_month
      )
  );
  return l_ids;
}
std::vector<std::tuple<project, std::string>> get_projects_and_status_name_by_project_name(
    const std::string& in_project_name
) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_projects = l_sql.impl_->storage_any_.select(
      columns(object<project>(true), &project_status::name_), from<project>(),
      join<project_status>(on(c(&project::project_status_id_) == c(&project_status::uuid_id_))),
      where(in_project_name.empty() || c(&project::name_) == in_project_name), order_by(&project_status::name_)
  );
  return l_projects;
}
std::optional<std::int64_t> get_project_person_id_by_project_id_and_person_id(
    const uuid& in_project_id, const uuid& in_person_id
) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_ids = l_sql.impl_->storage_any_.select(
      &project_person_link::id_,
      where(
          c(&project_person_link::project_id_) == in_project_id && c(&project_person_link::person_id_) == in_person_id
      )
  );
  return !l_ids.empty() ? std::optional{l_ids.front()} : std::optional<std::int64_t>{std::nullopt};
}
std::optional<std::int64_t> get_project_status_automation_id_by_project_id_and_status_id(
    const uuid& in_project_id, const uuid& in_status_id
) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_ids = l_sql.impl_->storage_any_.select(
      &project_status_automation_link::id_,
      where(
          c(&project_status_automation_link::project_id_) == in_project_id &&
          c(&project_status_automation_link::status_automation_id_) == in_status_id
      )
  );
  return !l_ids.empty() ? std::optional{l_ids.front()} : std::optional<std::int64_t>{std::nullopt};
}
std::vector<entity> get_entities_by_person_id_and_is_admin_and_is_shared(
    const uuid& in_person_id, bool in_is_admin, bool in_is_shared
) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_temporal_type_ids = l_sql.get_temporal_type_ids();
  auto l_dynamic_where     = dynamic_where(l_sql.impl_->storage_any_);
  l_dynamic_where.push_back(not_in(&entity::entity_type_id_, l_temporal_type_ids));
  if (!in_is_admin) {
    l_dynamic_where.push_back(
        in(&entity::project_id_,
           select(&project_person_link::project_id_, where(c(&project_person_link::person_id_) == in_person_id)))
    );
  }
  l_dynamic_where.push_back(c(&entity::is_shared_) == in_is_shared);

  auto l_entt = l_sql.impl_->storage_any_.get_all<entity>(where(l_dynamic_where));
  return l_entt;
}
std::vector<entity_fts> search_entities_fts_by_keyword(
    const std::string& in_keyword, const uuid& in_project_id, const std::int64_t in_limit, const std::int64_t in_offset
) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  using entity_fts_hidden = fts5::hidden_fields_of<entity_fts>;
  auto l_t                = l_sql.get_temporal_type_ids();
  auto l_re               = l_sql.impl_->storage_any_.select(
      object<entity_fts>(),
      where(
          match(entity_fts_hidden::any_field, in_keyword) && not_in(&entity_fts::entity_type_id_, l_t) &&
          c(&entity_fts::project_id_) == in_project_id
      ),
      order_by(rank()).asc(), limit(in_offset, in_limit)
  );
  return l_re;
}

std::vector<std::tuple<entity, task, entity_asset_extend, asset_type, uuid>>
make_with_tasks_sql_result_t::operator()() const {
  using namespace sqlite_orm;

  auto l_sql               = get_sqlite_database();
  auto l_temporal_type_ids = l_sql.get_temporal_type_ids();

  auto l_dynamic_where     = dynamic_where(l_sql.impl_->storage_any_);
  l_dynamic_where.push_back(not_in(&entity::entity_type_id_, l_temporal_type_ids));
  if (person_.role_ == person_role_type::outsource)
    l_dynamic_where.push_back(
        in(&entity::uuid_id_, select(
                                  &outsource_studio_authorization::entity_id_,
                                  where(c(&outsource_studio_authorization::studio_id_) == person_.studio_id_)
                              ))
    );
  if (!id_.is_nil()) {
    l_dynamic_where.push_back(c(&entity::uuid_id_) == id_);
  } else {
    if (!project_id_.is_nil()) l_dynamic_where.push_back(c(&entity::project_id_) == project_id_);
    if (!entity_type_id_.empty()) l_dynamic_where.push_back(in(&entity::entity_type_id_, entity_type_id_));
    if (!ji_du_filter_.empty() || ji_du_filter_is_null)
      l_dynamic_where.push_back(
          in(&entity_asset_extend::ji_du_, ji_du_filter_) ||
          (ji_du_filter_is_null && is_null(&entity_asset_extend::ji_du_))
      );
    if (!ji_shu_lie_filter_.empty() || ji_shu_lie_filter_is_null)
      l_dynamic_where.push_back(
          in(&entity_asset_extend::ji_shu_lie_, ji_shu_lie_filter_) ||
          (ji_shu_lie_filter_is_null && is_null(&entity_asset_extend::ji_shu_lie_))
      );
    if (!task_status_id_filter_.empty()) l_dynamic_where.push_back(in(&task::task_status_id_, task_status_id_filter_));
    if (!person_id_filter_.empty()) l_dynamic_where.push_back(in(&assignees_table::person_id_, person_id_filter_));
    using entity_fts_hidden = fts5::hidden_fields_of<entity_fts>;
    auto l_t                = l_sql.get_temporal_type_ids();

    if (!search_key_.empty())
      l_dynamic_where.push_back(
          in(&entity::uuid_id_,
             select(
                 &entity_fts::entity_id_,
                 where(
                     match(entity_fts_hidden::any_field, search_key_) && not_in(&entity_fts::entity_type_id_, l_t) &&
                     c(&entity_fts::project_id_) == project_id_
                 ),
                 order_by(rank()).asc()
             ))
      );
    if (!scenes_.empty() || scenes_is_null)
      l_dynamic_where.push_back(
          in(&entity_asset_extend::chang_ci_, scenes_) || (scenes_is_null && is_null(&entity_asset_extend::chang_ci_))
      );
  }

  auto l_outsource_select = select(
      &outsource_studio_authorization::entity_id_,
      where(c(&outsource_studio_authorization::studio_id_) == person_.studio_id_)
  );
  auto l_rows = l_sql.impl_->storage_any_.select(
      columns(
          object<entity>(true), object<task>(true), object<entity_asset_extend>(true), object<asset_type>(true),
          &assignees_table::person_id_
      ),
      from<entity>(), join<asset_type>(on(c(&entity::entity_type_id_) == c(&asset_type::uuid_id_))),
      left_outer_join<task>(on(c(&entity::uuid_id_) == c(&task::entity_id_))),
      left_outer_join<assignees_table>(on(c(&assignees_table::task_id_) == c(&task::uuid_id_))),
      left_outer_join<entity_asset_extend>(on(c(&entity_asset_extend::entity_id_) == c(&entity::uuid_id_))),
      where(l_dynamic_where), multi_order_by(order_by(&asset_type::name_), order_by(&entity::name_)),
      limit(offset_, limit_)
  );
  return l_rows;
}

}  // namespace doodle::sqlite_select