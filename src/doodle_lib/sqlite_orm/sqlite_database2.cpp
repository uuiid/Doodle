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
#include "sqlite_select_data.h"
#include <spdlog/spdlog.h>
#include <sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>
#include <vector>

namespace doodle::sqlite_select {

std::vector<std::tuple<uuid, std::string>> get_project_ids_and_names() {
  auto& l_sql = get_sqlite_database();
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
  auto& l_sql = get_sqlite_database();
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
  auto& l_sql = get_sqlite_database();
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
  auto& l_sql = get_sqlite_database();
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
  auto& l_sql = get_sqlite_database();
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
  auto& l_sql = get_sqlite_database();
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
  auto& l_sql = get_sqlite_database();
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
  auto& l_sql = get_sqlite_database();
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

  auto& l_sql              = get_sqlite_database();
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

    if (!search_key_.empty()) {
      if (search_key_.starts_with("name_:"))
        l_dynamic_where.push_back(like(&entity::name_, fmt::format("%{}%", search_key_.substr(6))));
      else if (search_key_.starts_with("description_:"))
        l_dynamic_where.push_back(like(&entity::description_, fmt::format("%{}%", search_key_.substr(13))));
      else if (search_key_.starts_with("bian_hao_:"))
        l_dynamic_where.push_back(like(&entity_asset_extend::bian_hao_, fmt::format("%{}%", search_key_.substr(10))));
      else if (search_key_.starts_with("pin_yin_ming_cheng_:"))
        l_dynamic_where.push_back(
            like(&entity_asset_extend::pin_yin_ming_cheng_, fmt::format("%{}%", search_key_.substr(20)))
        );
      else
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
    }
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
actions_projects_casting_copy_select get_actions_projects_casting_copy_select(
    const uuid& in_source_project_id, const uuid& in_target_project_id
) {
  auto& l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  actions_projects_casting_copy_select l_ret{};
  constexpr auto shot     = "shot"_alias.for_<entity>();
  constexpr auto sequence = "sequence"_alias.for_<entity>();
  l_ret.source_casting_   = l_sql.impl_->storage_any_.get_all<entity_link>(
      join<shot>(on(c(&entity_link::entity_in_id_) == c(shot->*&entity::uuid_id_))),
      join<sequence>(on(c(shot->*&entity::parent_id_) == c(sequence->*&entity::uuid_id_))),
      where(c(sequence->*&entity::uuid_id_) == in_source_project_id)
  );
  // 查找 shot
  l_ret.source_shots_ = l_sql.impl_->storage_any_.get_all<entity>(
      where(c(&entity::parent_id_) == in_source_project_id && c(&entity::canceled_) != true)
  );
  l_ret.target_shots_ = l_sql.impl_->storage_any_.get_all<entity>(
      where(c(&entity::parent_id_) == in_target_project_id && c(&entity::canceled_) != true)
  );
  return l_ret;
}
actions_projects_sequences_casting_ue_assembly_harvest_select_t
actions_projects_sequences_casting_ue_assembly_harvest_select_t::get(
    const uuid& in_project_id, const uuid& in_sequence_id
) {
  auto& l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  constexpr auto shot     = "shot"_alias.for_<entity>();
  constexpr auto sequence = "sequence"_alias.for_<entity>();
  // 直接获取当集资产, 后续不使用数据库进行匹配, 人工匹配
  auto l_ass_all          = l_sql.impl_->storage_any_.select(
      columns(object<entity_asset_extend>(), &entity::entity_type_id_), from<entity_asset_extend>(),
      join<entity>(on(c(&entity::uuid_id_) == c(&entity_asset_extend::entity_id_))),
      where(
          c(&entity::project_id_) == in_project_id &&
          in(&entity_asset_extend::entity_id_,
                      select(
                 &entity_link::entity_out_id_, from<entity_link>(),
                 join<shot>(on(c(&entity_link::entity_in_id_) == c(shot->*&entity::uuid_id_))),
                 join<sequence>(on(c(shot->*&entity::parent_id_) == c(sequence->*&entity::uuid_id_))),
                 where(c(sequence->*&entity::uuid_id_) == in_sequence_id && !c(shot->*&entity::canceled_))
             ))
      )
  );

  std::vector<uuid> l_ass_shot_ids{};
  auto l_shot_and_ext = l_sql.impl_->storage_any_.select(
      columns(object<entity>(true), object<entity_shot_extend>(true)), from<entity>(),
      join<entity_shot_extend>(on(c(&entity::uuid_id_) == c(&entity_shot_extend::entity_id_))),
      join<sequence>(on(c(&entity::parent_id_) == c(sequence->*&entity::uuid_id_))),
      where(c(sequence->*&entity::uuid_id_) == in_sequence_id && !c(&entity::canceled_))
  );
  for (auto&& [l_shot, l_shot_ext] : l_shot_and_ext) {
    l_ass_shot_ids.emplace_back(l_shot.uuid_id_);
  }

  // 查询所有的子镜头和用到的资产
  auto l_ass_link = l_sql.impl_->storage_any_.select(
      columns(object<entity_link>(), object<entity_asset_extend>(), &entity::entity_type_id_), from<entity_link>(),
      join<entity>(on(c(&entity::uuid_id_) == c(&entity_link::entity_out_id_))),
      join<entity_asset_extend>(on(c(&entity::uuid_id_) == c(&entity_asset_extend::entity_id_))),
      where(in(&entity_link::entity_in_id_, l_ass_shot_ids))
  );
  return actions_projects_sequences_casting_ue_assembly_harvest_select_t{
      .ass_all_ = l_ass_all, .shot_and_ext_ = l_shot_and_ext, .ass_link = l_ass_link
  };
}

get_get_entities_and_tasks_select_t get_get_entities_and_tasks_select_t::get(
    const person& in_person, const uuid& in_project_id, const uuid& in_entity_type_id, std::int32_t in_offset,
    std::int32_t in_limit
) {
  auto& l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_outsource_select = select(
      &outsource_studio_authorization::entity_id_,
      where(c(&outsource_studio_authorization::studio_id_) == in_person.studio_id_)
  );

  auto l_subscriptions_for_user = l_sql.get_person_subscriptions(in_person, in_project_id, in_entity_type_id);
  auto sequence                 = "sequence"_alias.for_<entity>();

  auto l_rows                   = l_sql.impl_->storage_any_.select(
      columns(object<entity>(true), object<task>(true), &assignees_table::person_id_), from<entity>(),
      left_outer_join<task>(on(c(&entity::uuid_id_) == c(&task::entity_id_))),
      left_outer_join<assignees_table>(on(c(&assignees_table::task_id_) == c(&task::uuid_id_))),
      where(
          c(&entity::entity_type_id_) == in_entity_type_id &&
          ((in_project_id.is_nil() || c(&entity::project_id_) == in_project_id) &&
           (in_person.role_ != person_role_type::outsource || in(&entity::uuid_id_, l_outsource_select)))

      ),
      multi_order_by(order_by(&entity::name_)), limit(in_offset, in_limit)
  );
  auto l_cout = l_sql.impl_->storage_any_.select(
      columns(sequence->*&entity::uuid_id_, count(sequence->*&entity::uuid_id_)), from<entity>(),
      join<sequence>(on(c(&entity::parent_id_) == c(sequence->*&entity::uuid_id_))),
      where(c(&entity::project_id_) == in_project_id), group_by(sequence->*&entity::uuid_id_)
  );
  return get_get_entities_and_tasks_select_t{.entity_and_task_and_person_id_ = l_rows, .sequence_and_cout_ = l_cout};
}

std::vector<entity> get_entity_by_episode_id_and_project_id_and_name(
    const uuid& type_id_, const uuid& in_episode_id, const uuid& in_project_id, const std::string& in_name
) {
  auto& l_sql = get_sqlite_database();
  using namespace sqlite_orm;

  auto l_dynamic_where = dynamic_where(l_sql.impl_->storage_any_);
  if (!in_name.empty()) l_dynamic_where.push_back(c(&entity::name_) == in_name);
  if (!in_project_id.is_nil()) l_dynamic_where.push_back(c(&entity::project_id_) == in_project_id);
  if (!in_episode_id.is_nil()) l_dynamic_where.push_back(c(&entity::parent_id_) == in_episode_id);
  DOODLE_CHICK(!type_id_.is_nil(), "类别id不可空")
  l_dynamic_where.push_back(c(&entity::entity_type_id_) == type_id_);

  auto l_sq_list = l_sql.impl_->storage_any_.get_all<entity>(where(l_dynamic_where));
  return l_sq_list;
}
bool task_exit_by_entity_id_and_task_type_id(const uuid& in_entity_id, const uuid& in_task_type_id) {
  auto& l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_count = l_sql.impl_->storage_any_.count<task>(
      where(c(&task::entity_id_) == in_entity_id && c(&task::task_type_id_) == in_task_type_id)
  );
  return l_count > 0;
}

std::vector<sd2_select_task_t> get_tasks_and_entity_for_ai_studio(const uuid& in_ai_studio_id) {
  auto& l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_r = l_sql.impl_->storage_any_.select(
      columns(object<sd2::task>(true), object<entity>(true), object<task>(true), object<entity_shot_extend>(true)),
      from<sd2::task>(), left_outer_join<task>(on(c(&sd2::task::task_id_) == c(&task::uuid_id_))),
      left_outer_join<entity>(on(c(&task::entity_id_) == c(&entity::uuid_id_))),
      left_outer_join<entity_shot_extend>(on(c(&entity_shot_extend::entity_id_) == c(&entity::uuid_id_))),
      where(c(&sd2::task::ai_studio_id_) == in_ai_studio_id)
  );
  std::vector<sd2_select_task_t> l_result{};
  l_result.reserve(l_r.size());
  for (auto&& [l_sd2_task, l_entity, l_task, l_entity_shot_extend] : l_r)
    l_result.emplace_back(l_sd2_task, l_entity, l_task, l_entity_shot_extend);
  return l_result;
}

std::vector<sd2_select_task_t> get_tasks_and_entity_for_person(const uuid& in_person_id) {
  auto& l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_r = l_sql.impl_->storage_any_.select(
      columns(object<sd2::task>(true), object<entity>(true), object<task>(true), object<entity_shot_extend>(true)),
      from<sd2::task>(), left_outer_join<task>(on(c(&sd2::task::task_id_) == c(&task::uuid_id_))),
      left_outer_join<entity>(on(c(&task::entity_id_) == c(&entity::uuid_id_))),
      left_outer_join<entity_shot_extend>(on(c(&entity_shot_extend::entity_id_) == c(&entity::uuid_id_))),
      where(c(&sd2::task::user_id_) == in_person_id)
  );
  std::vector<sd2_select_task_t> l_result{};
  l_result.reserve(l_r.size());
  for (auto&& [l_sd2_task, l_entity, l_task, l_entity_shot_extend] : l_r)
    l_result.emplace_back(l_sd2_task, l_entity, l_task, l_entity_shot_extend);
  return l_result;
}
std::vector<sd2::task> get_task_for_shot_task_id(const uuid& in_task_id, const uuid& in_ai_studio_id) {
  auto& l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_r = l_sql.impl_->storage_any_.get_all<sd2::task>(
      where(c(&sd2::task::shot_uuid_id_) == in_task_id && c(&sd2::task::ai_studio_id_) == in_ai_studio_id)
  );
  return l_r;
}
}  // namespace doodle::sqlite_select