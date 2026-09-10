//
// Created by TD on 25-7-12.
//
// 深度估计参考 — 遵循 seedance2_subproject_entity_reference 模式
// POST: 上传视频 → 创建 ai_preview_file + ai_entity_reference_preview → 存盘 → 返回 JSON
//

#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/seedance2/ai_generate_entity.h>
#include <doodle_core/metadata/seedance2/ai_preview_file.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/computers.h>
#include <doodle_lib/http_method/seedance2/reg.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/asio/consign.hpp>
#include <boost/asio/post.hpp>

#include "core/global_function.h"
#include <core/http/http_function.h>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <utility>

namespace doodle::http::seedance2 {
namespace sd2 = doodle::seedance2;

// POST /api/seedance2/subproject/{subproject_id}/entity/{entity_id}/depth
// 上传视频 → 创建 ai_preview_file + ai_entity_reference_preview → 存盘 → 生成缩略图
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(doodle_ai_depth_estimation_video, post) {
  person_.check_subproject_access(subproject_id_);
  person_.check_not_outsourcer();

  auto l_sql      = get_sqlite_database();
  auto l_entity   = std::make_shared<sd2::ai_generate_entity>(l_sql.get_by_uuid<sd2::ai_generate_entity>(entity_id_));
  auto l_file     = in_handle->get_file();
  auto l_ext      = l_file.extension().string();
  auto l_is_video = l_ext == ".mp4" || l_ext == ".mov" || l_ext == ".avi";

  DOODLE_CHICK(l_is_video, "请上传视频文件 (.mp4/.mov/.avi)");

  // 1. insert::values() 自动生成 uuid_id_（insert.h:92-93）
  auto l_preview = std::make_shared<sd2::ai_preview_file>();
  using namespace orm;
  l_preview->extension_         = l_ext;
  auto l_install_1              = insert(l_sql).into<sd2::ai_preview_file>().values(*l_preview);

  auto l_ref                    = std::make_shared<sd2::ai_entity_reference_preview>();
  l_ref->ai_generate_entity_id_ = entity_id_;
  l_ref->preview_file_          = l_preview->uuid_id_;  // values() 后 uuid 已生成
  auto l_install_2              = insert(l_sql).into<sd2::ai_entity_reference_preview>().values(*l_ref);

  // 2. 创建分布式任务
  auto l_task = std::make_shared<server_task_info>();
  l_task->type_      = server_task_info_type::depth_estimation;
  l_task->status_    = server_task_info_status::submitted;
  l_task->task_id_   = entity_id_;
  l_task->submitter_ = person_.person_.uuid_id_;
  l_task->command_   = nlohmann::json{{"preview_id", l_preview->uuid_id_}};
  auto l_install_3   = insert(l_sql).into<server_task_info>().values(*l_task);

  // 3. 一次提交三个 insert
  co_await l_sql.run_sql(l_install_1, l_install_2, l_install_3);

  // 4. 存储路径 — 原始视频暂存临时文件
  auto& l_ctx           = g_ctx().get<kitsu_ctx_t>();
  auto l_file_picture   = l_ctx.get_sd2_pictures_file(l_preview->uuid_id_, l_ext);
  auto l_file_thumbnail = l_ctx.get_sd2_thumbnail_file(l_preview->uuid_id_);
  auto l_file_tmp       = l_file_picture;
  l_file_tmp.replace_extension(".upload_tmp.mp4");

  if (auto l_p = l_file_picture.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);

  FSys::rename(l_file, l_file_tmp);

  co_await computers_assign_task::get_instance().run_next_task();

  co_return in_handle->make_msg(nlohmann::json{{"reference", *l_ref}, {"preview", *l_preview}});
}

// GET /api/seedance2/depth/{depth_id} — 工作机下载输入视频
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(doodle_ai_depth_estimation_file, get) {
  auto l_sql     = get_sqlite_database();
  auto l_preview = l_sql.get_by_uuid<sd2::ai_preview_file>(depth_id_);
  auto& l_ctx    = g_ctx().get<kitsu_ctx_t>();
  auto l_file_picture = l_ctx.get_sd2_pictures_file(depth_id_, l_preview.extension_);
  auto l_file_tmp     = l_file_picture;
  l_file_tmp.replace_extension(".upload_tmp.mp4");
  DOODLE_CHICK_HTTP(FSys::exists(l_file_tmp), not_found, "输入文件不存在");
  co_return in_handle->make_msg(l_file_tmp, kitsu::mime_type(l_file_tmp.extension()));
}

// PUT /api/seedance2/depth/{depth_id} — 工作机上传深度估计结果视频
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(doodle_ai_depth_estimation_file, put) {
  auto l_file = in_handle->get_file();
  DOODLE_CHICK_HTTP(!l_file.empty() && FSys::exists(l_file), bad_request, "必须上传深度估计结果视频");

  auto l_sql     = get_sqlite_database();
  auto l_preview = l_sql.get_by_uuid<sd2::ai_preview_file>(depth_id_);
  auto& l_ctx    = g_ctx().get<kitsu_ctx_t>();

  auto l_file_picture   = l_ctx.get_sd2_pictures_file(depth_id_, l_preview.extension_);
  auto l_file_thumbnail = l_ctx.get_sd2_thumbnail_file(depth_id_);

  if (auto l_p = l_file_picture.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  if (auto l_p = l_file_thumbnail.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);

  FSys::rename(l_file, l_file_picture);

  // 清理临时上传文件
  auto l_file_tmp = l_file_picture;
  l_file_tmp.replace_extension(".upload_tmp.mp4");
  if (FSys::exists(l_file_tmp)) FSys::remove(l_file_tmp);

  // 查找关联的 ai_entity_reference_preview 并广播完成
  using namespace orm;
  auto l_ref = select(l_sql)
                   .columns(object<sd2::ai_entity_reference_preview>())
                   .from<sd2::ai_entity_reference_preview>()
                   .where(c(&sd2::ai_entity_reference_preview::preview_file_) == depth_id_)
                   .limit(1)()
                   .to_optional();

  if (l_ref) {
    socket_io::broadcast(
        socket_io::seedance2_entity_reference_new_broadcast_t{
            .reference_id_ = l_ref->uuid_id_, .entity_id_ = l_ref->ai_generate_entity_id_, .preview_file_id_ = depth_id_
        }
    );
  }

  co_return in_handle->make_msg_204();
}

}  // namespace doodle::http::seedance2