//
// Created by TD on 25-8-15.
//
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/metadata/entity.h"
#include "doodle_core/metadata/entity_type.h"
#include "doodle_core/metadata/project.h"
#include "doodle_core/metadata/task.h"
#include "doodle_core/metadata/task_type.h"
#include <doodle_core/metadata/working_file.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/entity_path.h>
#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/scan_assets.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_front_end.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include <boost/exception/diagnostic_information.hpp>

#include <nlohmann/json_fwd.hpp>
#include <range/v3/action/push_back.hpp>
#include <range/v3/view/unique.hpp>
#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <vector>
#include <winsock2.h>

namespace doodle::http {

boost::asio::awaitable<boost::beast::http::message_generator> actions_tasks_working_file::post(
    session_data_ptr in_handle
) {
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_task = l_sql.get_by_uuid<task>(id_);

  co_await scan_assets::scan_task_async(l_task);
  co_return in_handle->make_msg(nlohmann::json{} = l_sql.get_working_file_by_task(id_));
}

boost::asio::awaitable<boost::beast::http::message_generator> actions_tasks_working_file::delete_(
    session_data_ptr in_handle
) {
  auto l_sql          = g_ctx().get<sqlite_database>();
  auto l_task         = l_sql.get_by_uuid<task>(id_);
  auto l_work_file_id = in_handle->get_json().at("id").get<uuid>();
  using namespace sqlite_orm;
  if (auto l_work_file_link = l_sql.impl_->storage_any_.get_all<working_file_task_link>(where(
          c(&working_file_task_link::working_file_id_) == l_work_file_id && c(&working_file_task_link::task_id_) == id_
      ));
      !l_work_file_link.empty()) {
    co_await l_sql.remove<working_file_task_link>(l_work_file_link.front().id_);
  }

  co_return in_handle->make_msg_204();
}

bool entity_has_simulation_asset(const uuid& in_entity_id) {
  using namespace sqlite_orm;
  return g_ctx().get<sqlite_database>().impl_->storage_any_.count<task>(
             where(c(&task::entity_id_) == in_entity_id && c(&task::task_type_id_) == task_type::get_simulation_id())
         ) > 0;
}

std::vector<working_file_and_link> create_character_working_files(
    const project& in_project, const entity& in_entity, const entity_asset_extend& in_entity_asset_extend
) {
  std::vector<working_file_and_link> l_working_files{};
  l_working_files.reserve(4);
  l_working_files.emplace_back(
      working_file_and_link{
          working_file{
              .description_ = "角色UE工作文件",
              .path_        = get_entity_character_ue_path(in_project, in_entity_asset_extend) /
                       get_entity_character_ue_name(in_entity_asset_extend),
              .software_type_ = software_enum::unreal_engine,

          },
          in_entity.uuid_id_,
      }
  );
  l_working_files.emplace_back(
      working_file_and_link{
          working_file{
              .description_ = "角色Maya rig工作文件",
              .path_        = get_entity_character_rig_maya_path(in_project, in_entity_asset_extend) /
                       get_entity_character_rig_maya_name(in_entity_asset_extend),
              .software_type_ = software_enum::maya_rig,
          },
          in_entity.uuid_id_,
      }
  );
  if (entity_has_simulation_asset(in_entity.uuid_id_))
    l_working_files.emplace_back(
        working_file_and_link{
            working_file{
                .description_ = "角色Maya 解算工作文件",
                .path_        = get_entity_simulation_asset_path(in_project) /
                         get_entity_simulation_character_asset_name(in_entity_asset_extend),
                .software_type_ = software_enum::maya_sim,
            },
            in_entity.uuid_id_,
        }
    );
  return l_working_files;
}

std::vector<working_file_and_link> create_prop_working_files(
    const project& in_project, const entity& in_entity, const entity_asset_extend& in_entity_asset_extend
) {
  std::vector<working_file_and_link> l_working_files{};
  l_working_files.reserve(3);
  l_working_files.emplace_back(
      working_file_and_link{
          working_file{
              .description_ = "道具UE工作文件",
              .path_        = get_entity_prop_ue_path(in_project, in_entity_asset_extend) /
                       get_entity_prop_ue_name(in_entity_asset_extend),
              .software_type_ = software_enum::unreal_engine,
          },
          in_entity.uuid_id_,
      }
  );

  l_working_files.emplace_back(
      working_file_and_link{
          working_file{
              .description_ = "道具Maya 模型工作文件",
              .path_        = get_entity_prop_model_maya_path(in_project, in_entity_asset_extend) /
                       get_entity_prop_model_maya_name(in_entity_asset_extend),
              .software_type_ = software_enum::maya,
          },
          in_entity.uuid_id_,
      }
  );

  l_working_files.emplace_back(
      working_file_and_link{
          working_file{
              .description_ = "道具Maya rig工作文件",
              .path_        = get_entity_prop_rig_maya_path(in_project, in_entity_asset_extend) /
                       get_entity_prop_rig_maya_name(in_entity_asset_extend),
              .software_type_ = software_enum::maya_rig,
          },
          in_entity.uuid_id_,
      }
  );
  if (entity_has_simulation_asset(in_entity.uuid_id_))
    l_working_files.emplace_back(
        working_file_and_link{
            working_file{
                .description_ = "道具Maya 解算工作文件",
                .path_        = get_entity_simulation_asset_path(in_project) /
                         get_entity_simulation_prop_asset_name(in_entity_asset_extend),
                .software_type_ = software_enum::maya_sim,
            },
            in_entity.uuid_id_,
        }
    );
  return l_working_files;
}
std::vector<working_file_and_link> create_ground_working_files(
    const project& in_project, const entity& in_entity, const entity_asset_extend& in_entity_asset_extend
) {
  std::vector<working_file_and_link> l_working_files{};
  l_working_files.reserve(2);
  l_working_files.emplace_back(
      working_file_and_link{
          working_file{
              .description_ = "场景UE工作文件",
              .path_        = get_entity_ground_ue_path(in_project, in_entity_asset_extend) /
                       get_entity_ground_ue_map_name(in_entity_asset_extend),
              .software_type_ = software_enum::unreal_engine,
          },
          in_entity.uuid_id_,
      }
  );
  l_working_files.emplace_back(
      working_file_and_link{
          working_file{
              .description_ = "场景UE Sk文件",
              .path_        = get_entity_ground_ue_path(in_project, in_entity_asset_extend) /
                       get_entity_ground_ue_sk_name(in_entity_asset_extend),
              .software_type_ = software_enum::unreal_engine_sk,
          },
          in_entity.uuid_id_,
      }
  );
  l_working_files.emplace_back(
      working_file_and_link{
          working_file{
              .description_ = "场景Maya alembic 工作文件",
              .path_        = get_entity_ground_model_maya_path(in_project, in_entity_asset_extend) /
                       get_entity_ground_alembic_name(in_entity_asset_extend),
              .software_type_ = software_enum::alembic,
          },
          in_entity.uuid_id_,
      }
  );
  l_working_files.emplace_back(
      working_file_and_link{
          working_file{
              .description_ = "场景Maya rig工作文件",
              .path_        = get_entity_ground_model_maya_path(in_project, in_entity_asset_extend) /
                       get_entity_ground_rig_name(in_entity_asset_extend),
              .software_type_ = software_enum::maya_rig,
          },
          in_entity.uuid_id_,
      }
  );

  return l_working_files;
}

std::vector<working_file_and_link> get_working_files_for_entity(
    const uuid& in_project_id, const uuid& in_shot_id, const uuid& in_sequence_id
) {
  auto l_sql = g_ctx().get<sqlite_database>();

  std::vector<working_file_and_link> l_working_files{};
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
  auto l_prj = l_sql.get_by_uuid<project>(in_project_id);
  for (auto&& [l_entity, l_entity_asset_extend] : l_assets) {
    auto l_begin = l_working_files.size();
    if (l_entity.entity_type_id_ == asset_type::get_character_id()) {
      l_working_files |=
          ranges::actions::push_back(create_character_working_files(l_prj, l_entity, l_entity_asset_extend));
    } else if (l_entity.entity_type_id_ == asset_type::get_prop_id() ||
               l_entity.entity_type_id_ == asset_type::get_effect_id()) {
      l_working_files |= ranges::actions::push_back(create_prop_working_files(l_prj, l_entity, l_entity_asset_extend));
    } else if (l_entity.entity_type_id_ == asset_type::get_ground_id()) {
      l_working_files |=
          ranges::actions::push_back(create_ground_working_files(l_prj, l_entity, l_entity_asset_extend));
    }
    for (auto i = l_begin; i < l_working_files.size(); ++i) {
      l_working_files[i].entity_type_id_ = l_entity.entity_type_id_;
    }
  }
  for (auto&& i : l_working_files) {
    i.name_ = i.path_.has_extension() ? i.path_.filename().string() : std::string{};
    if (!FSys::exists(l_prj.path_ / i.path_)) i.path_ = FSys::path{};
  }

  return l_working_files;
}

boost::asio::awaitable<boost::beast::http::message_generator> actions_projects_shots_working_file::get(
    session_data_ptr in_handle
) {
  auto l_sql = g_ctx().get<sqlite_database>();
  if (l_sql.uuid_to_id<entity>(id_) == 0)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未知的镜头 id ");
  if (l_sql.uuid_to_id<project>(project_id_) == 0)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未知的项目 id ");

  co_return in_handle->make_msg(nlohmann::json{} = get_working_files_for_entity(project_id_, id_, {}));
}

boost::asio::awaitable<boost::beast::http::message_generator> actions_projects_sequences_working_file::get(
    session_data_ptr in_handle
) {
  auto l_sql = g_ctx().get<sqlite_database>();
  if (l_sql.uuid_to_id<entity>(id_) == 0)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未知的序列 id ");
  if (l_sql.uuid_to_id<project>(project_id_) == 0)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未知的项目 id ");
  co_return in_handle->make_msg(nlohmann::json{} = get_working_files_for_entity(project_id_, {}, id_));
}

boost::asio::awaitable<boost::beast::http::message_generator> actions_tasks_working_file::get(
    session_data_ptr in_handle
) {
  auto l_sql = g_ctx().get<sqlite_database>();
  if (l_sql.uuid_to_id<task>(id_) == 0)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未知的任务 id ");
  auto l_task = l_sql.get_by_uuid<task>(id_);
  auto l_prj  = l_sql.get_by_uuid<project>(l_task.project_id_);
  using namespace sqlite_orm;
  auto l_r = l_sql.impl_->storage_any_.select(
      columns(object<entity>(true), object<entity_asset_extend>(true)), from<entity>(),
      left_outer_join<entity_asset_extend>(on(c(&entity_asset_extend::entity_id_) == c(&entity::uuid_id_))),
      where(c(&entity::uuid_id_) == l_task.entity_id_)
  );
  std::vector<working_file_and_link> l_working_files{};
  if (auto&& [l_entity, l_entity_asset_extend] = l_r.front();
      l_entity.entity_type_id_ == asset_type::get_character_id()) {
    l_working_files = create_character_working_files(l_prj, l_entity, l_entity_asset_extend);
  } else if (l_entity.entity_type_id_ == asset_type::get_prop_id() ||
             l_entity.entity_type_id_ == asset_type::get_effect_id()) {
    l_working_files = create_prop_working_files(l_prj, l_entity, l_entity_asset_extend);
  } else if (l_entity.entity_type_id_ == asset_type::get_ground_id()) {
    l_working_files = create_ground_working_files(l_prj, l_entity, l_entity_asset_extend);
  }
  for (auto&& i : l_working_files) {
    i.name_ = i.path_.has_extension() ? i.path_.filename().string() : std::string{};
    if (!FSys::exists(l_prj.path_ / i.path_)) i.path_ = FSys::path{};
  }
  co_return in_handle->make_msg(nlohmann::json{} = l_working_files);
}
}  // namespace doodle::http