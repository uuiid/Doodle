#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/task.h"
#include "doodle_core/metadata/task_type.h"
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/http_method/kitsu/preview.h>
#include <doodle_lib/http_method/seed_email.h>

#include <memory>
#include <sqlite_orm/sqlite_orm.h>

namespace doodle::http {

namespace {
// 先合成视频, 再生成预览

auto compose_video_impl(
    const FSys::path& in_path, const std::size_t& in_fps, const cv::Size& in_size,
    const std::shared_ptr<preview_file>& in_preview_file, const preview_file& in_target_preview_file
) {
  auto& l_ctx     = g_ctx().get<kitsu_ctx_t>();
  auto l_new_path = l_ctx.get_movie_source_file(in_preview_file->uuid_id_);
  if (auto l_p = l_new_path.parent_path(); !exists(l_p)) FSys::create_directories(l_p);
  auto l_target_path = l_ctx.get_movie_source_file(in_target_preview_file.uuid_id_);
}

}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_preview_files_compose_video, post) {
  auto l_file = in_handle->get_file();
  DOODLE_CHICK_HTTP(!l_file.empty() && FSys::exists(l_file), bad_request, "必须上传视频文件");
  auto l_sql          = g_ctx().get<sqlite_database>();
  auto l_preview_file = l_sql.get_by_uuid<preview_file>(preview_file_id_);
  auto l_task         = l_sql.get_by_uuid<task>(l_preview_file.task_id_);
  auto l_project      = l_sql.get_by_uuid<project>(l_task.project_id_);

  preview_file l_target_preview_file{};
  {
    using namespace sqlite_orm;
    auto l_preview_files = l_sql.impl_->storage_any_.get_all<preview_file>(
        join<task>(on(c(&preview_file::task_id_) == c(&task::uuid_id_)))

            ,
        where(
            c(&task::entity_id_) == l_task.entity_id_ &&
            in(&task::task_type_id_,
               {task_type::get_simulation_task_id(), task_type::get_lighting_id(), task_type::get_animation_id()})
        ),
        order_by(&preview_file::created_at_).desc(), limit(1)
    );
    DOODLE_CHICK_HTTP(!l_preview_files.empty(), bad_request, "没有找到相关的预览文件");
    // 选择最新的预览文件
    l_target_preview_file = l_preview_files.front();
  }
  // 开始合成视频
  auto l_prj_size               = l_project.get_resolution();
  l_preview_file.extension_     = "mp4";
  l_preview_file.original_name_ = l_file.stem().generic_string();
  l_preview_file.width_         = l_prj_size.first;
  l_preview_file.height_        = l_prj_size.second;
  auto l_preview_file_ptr       = std::make_shared<preview_file>(l_preview_file);
  co_await l_sql.update(l_preview_file_ptr);

  boost::asio::post(g_io_context(), [fps = l_project.fps_, l_prj_size, l_preview_file]() mutable {

  });

  co_return in_handle->make_msg(nlohmann::json{} = *l_preview_file_ptr);
}

}  // namespace doodle::http