//
// Created by TD on 25-8-15.
//
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/metadata/entity.h"
#include "doodle_core/metadata/entity_type.h"
#include "doodle_core/metadata/project.h"
#include "doodle_core/metadata/task.h"
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

namespace {
boost::asio::awaitable<void> scan_working_files() {
  auto l_sql = g_ctx().get<sqlite_database>();

  std::vector<std::int64_t> l_delete_ids{};
  for (auto&& l_f : l_sql.get_all<working_file>()) {
    if (!l_f.path_.empty() && !exists(l_f.path_)) l_delete_ids.emplace_back(l_f.id_);
  }
  if (!l_delete_ids.empty()) co_await l_sql.remove<working_file>(l_delete_ids);
  scan_assets::scan_result l_scan_result{};
  std::size_t l_count{};
  using namespace sqlite_orm;
  for (auto&& i : l_sql.impl_->storage_any_.iterate<task>(
           join<entity>(on(c(&task::entity_id_) == c(&entity::uuid_id_))),
           where(not_in(&entity::entity_type_id_, l_sql.get_temporal_type_ids()))
       )) {
    try {
      auto l_sc = scan_assets::scan_task(i);
      l_scan_result += *l_sc;
      ++l_count;
    } catch (...) {
      default_logger_raw()->error("扫描任务 {} 失败 {}", i.name_, boost::current_exception_diagnostic_information());
    }
  }
  default_logger_raw()->info("扫描完成, {} 个文件", l_count);
  co_await l_scan_result.install_async_sqlite();
  co_return;
}
}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> actions_working_files_scan_all::post(
    session_data_ptr in_handle
) {
  person_.check_admin();
  static std::atomic_bool g_begin_scan{false};
  if (g_begin_scan) co_return in_handle->make_msg_204();

  g_begin_scan = true;
  boost::asio::co_spawn(g_io_context(), scan_working_files(), [](const std::exception_ptr in_eptr) {
    try {
      if (in_eptr) std::rethrow_exception(in_eptr);
    } catch (const std::exception& e) {
      default_logger_raw()->error(e.what());
    };
    g_begin_scan = false;
  });

  co_return in_handle->make_msg_204();
}
boost::asio::awaitable<boost::beast::http::message_generator> actions_tasks_working_file::post(
    session_data_ptr in_handle
) {
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_task = l_sql.get_by_uuid<task>(id_);

  co_await scan_assets::scan_task_async(l_task);
  co_return in_handle->make_msg(nlohmann::json{} = l_sql.get_working_file_by_task(id_));
}
boost::asio::awaitable<boost::beast::http::message_generator> actions_tasks_working_file::get(
    session_data_ptr in_handle
) {
  auto l_sql = g_ctx().get<sqlite_database>();
  using namespace sqlite_orm;
  auto l_task = l_sql.get_working_file_by_task(id_);
  if (l_task.empty())
    in_handle->make_error_code_msg(boost::beast::http::status::not_found, "没有找到对应的working file");
  co_return in_handle->make_msg(nlohmann::json{} = l_task);
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
// template <class _Kty, class _Ty, class _Pr = std::less<_Kty>, class _Alloc = std::allocator<std::pair<const _Kty,
// _Ty>>> struct map_json_obj {
//   using map_t = std::map<_Kty, _Ty, _Pr, _Alloc>;
//   map_t l_map;
//   explicit map_json_obj(const map_t& in_map) : l_map(in_map) {}
//   // to json
//   friend void to_json(nlohmann::json& io_json, const map_json_obj<_Kty, _Ty, _Pr, _Alloc>& in_obj) {
//     for (auto&& i : in_obj.l_map) io_json[i.first] = i.second;
//   }
// };
template <class _Kty, class _Ty, class _Pr = std::less<_Kty>, class _Alloc = std::allocator<std::pair<const _Kty, _Ty>>>
nlohmann::json map_json_obj(nlohmann::json& io_json, const std::map<_Kty, _Ty, _Pr, _Alloc>& in_obj) {
  for (auto&& i : in_obj) io_json[i.first] = i.second;
  return io_json;
}
template <class _Kty, class _Ty, class _Pr = std::less<_Kty>, class _Alloc = std::allocator<std::pair<const _Kty, _Ty>>>
nlohmann::json map_json_obj(const std::map<_Kty, _Ty, _Pr, _Alloc>& in_obj) {
  nlohmann::json io_json{};
  for (auto&& i : in_obj) {
    if constexpr (std::is_same_v<std::decay_t<_Kty>, uuid>)
      io_json[fmt::to_string(i.first)] = i.second;
    else
      io_json[i.first] = i.second;

    // io_json.emplace_back(nlohmann::json{{i.first, i.second}});
  }
  return io_json;
}

std::map<uuid, std::vector<working_file>> get_working_files_for_tasks(const std::vector<uuid>& in_task_ids) {
  auto l_sql = g_ctx().get<sqlite_database>();
  std::map<uuid, std::vector<working_file>> l_work_map{};
  using namespace sqlite_orm;
  for (auto&& [l_w, l_task_id] : l_sql.impl_->storage_any_.select(
           columns(object<working_file>(true), &working_file_task_link::task_id_), from<working_file>(),
           join<working_file_task_link>(on(c(&working_file_task_link::working_file_id_) == c(&working_file::uuid_id_))),
           where(in(&working_file_task_link::task_id_, in_task_ids))
       )) {
    l_work_map[l_task_id].emplace_back(l_w);
  }
  return l_work_map;
}

boost::asio::awaitable<boost::beast::http::message_generator> actions_projects_tasks_working_file_many::post(
    session_data_ptr in_handle
) {
  auto l_sql = g_ctx().get<sqlite_database>();
  auto l_ids = in_handle->get_json().get<std::vector<uuid>>();
  std::map<uuid, std::vector<working_file>> l_work_map{};
  scan_assets::scan_result l_scan_result{};
  for (auto&& i : l_ids) {
    auto l_task = l_sql.get_by_uuid<task>(i);
    auto l_sc   = scan_assets::scan_task(l_task);
    l_scan_result += *l_sc;
  }
  co_await l_scan_result.install_async_sqlite();

  co_return in_handle->make_msg(map_json_obj(get_working_files_for_tasks(l_ids)));
}
boost::asio::awaitable<boost::beast::http::message_generator> actions_projects_entities_working_file_many::post(
    session_data_ptr in_handle
) {
  person_.check_project_manager(id_);
  auto l_sql = g_ctx().get<sqlite_database>();
  auto l_ids = in_handle->get_json().get<std::vector<uuid>>();
  std::vector<uuid> l_task_ids{};

  scan_assets::scan_result l_scan_result{};
  using namespace sqlite_orm;
  for (auto&& [i] : l_sql.impl_->storage_any_.select(
           columns(object<task>(true)), from<task>(), join<entity>(on(c(&task::entity_id_) == c(&entity::uuid_id_))),
           where(in(&entity::uuid_id_, l_ids))
       )) {
    auto l_sc = scan_assets::scan_task(i);
    l_scan_result += *l_sc;
    l_task_ids.emplace_back(i.uuid_id_);
  }
  co_await l_scan_result.install_async_sqlite();

  std::vector<working_file_and_link> l_working_files{};
  l_working_files.reserve(128);
  std::set<uuid> l_working_file_ids{};
  using namespace sqlite_orm;

  constexpr auto shot     = "shot"_alias.for_<entity>();
  constexpr auto sequence = "sequence"_alias.for_<entity>();
  for (auto&& [l_working_file, l_entity_id, l_task_id, l_task_type_id] : l_sql.impl_->storage_any_.select(
           columns(
               object<working_file>(true), &working_file_entity_link::entity_id_, &working_file_task_link::task_id_,
               &task::task_type_id_
           ),
           from<working_file>(),
           join<working_file_entity_link>(
               on(c(&working_file_entity_link::working_file_id_) == c(&working_file::uuid_id_))
           ),
           join<working_file_task_link>(on(c(&working_file_task_link::working_file_id_) == c(&working_file::uuid_id_))),
           join<task>(on(c(&task::uuid_id_) == c(&working_file_task_link::task_id_))),
           where(in(&working_file_task_link::task_id_, l_task_ids))
       )) {
    l_working_file_ids.emplace(l_working_file.uuid_id_);
    l_working_files.emplace_back(
        working_file_and_link{working_file{l_working_file}, l_entity_id, l_task_id, l_task_type_id}
    );
  }

  co_return in_handle->make_msg(l_working_files);
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
              .description_   = "角色Maya rig工作文件",
              .path_          = get_entity_character_rig_maya_path(in_project, in_entity_asset_extend),
              .software_type_ = software_enum::maya,
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
              .description_   = "道具Maya建模工作文件",
              .path_          = get_entity_prop_model_maya_path(in_project, in_entity_asset_extend),
              .software_type_ = software_enum::maya,
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
              .description_   = "场景UE工作文件",
              .path_          = get_entity_ground_ue_path(in_project, in_entity_asset_extend),
              .software_type_ = software_enum::unreal_engine,
          },
          in_entity.uuid_id_,
      }
  );
  l_working_files.emplace_back(
      working_file_and_link{
          working_file{
              .description_ = "场景Maya建模工作文件",
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
              .description_ = "场景Maya建模rig工作文件",
              .path_        = get_entity_ground_model_maya_path(in_project, in_entity_asset_extend) /
                       get_entity_ground_rig_name(in_entity_asset_extend),
              .software_type_ = software_enum::alembic,
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
  }
  for (auto&& i : l_working_files) {
    i.name_ = i.path_.has_extension() ? i.path_.filename().string() : std::string{};
    if (!FSys::exists(i.path_)) i.path_ = FSys::path{};
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

boost::asio::awaitable<boost::beast::http::message_generator> actions_tasks_working_file_reference::get(
    session_data_ptr in_handle
) {
  auto l_sql = g_ctx().get<sqlite_database>();
  if (l_sql.uuid_to_id<task>(id_) == 0)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未知的任务 id ");
  auto l_task = l_sql.get_by_uuid<task>(id_);
  using namespace sqlite_orm;
  auto l_r = l_sql.impl_->storage_any_.select(
      columns(object<working_file>(true), &task::task_type_id_), from<working_file>(),

      join<working_file_entity_link>(on(c(&working_file_entity_link::working_file_id_) == c(&working_file::uuid_id_))),
      join<working_file_task_link>(on(c(&working_file_task_link::working_file_id_) == c(&working_file::uuid_id_))),
      join<task>(on(c(&task::uuid_id_) == c(&working_file_task_link::task_id_))),
      where(c(&working_file_entity_link::entity_id_) == l_task.entity_id_)
  );
  std::vector<working_file_and_link> l_working_files{};
  l_working_files.reserve(l_r.size());
  for (auto&& [l_working_file, l_task_type_id] : l_r)
    l_working_files.emplace_back(working_file_and_link{working_file{l_working_file}, {}, {}, l_task_type_id});
  co_return in_handle->make_msg(nlohmann::json{} = l_working_files);
}

}  // namespace doodle::http