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

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/seedance2/reg.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <core/http/http_function.h>
#include <filesystem>
#include <opencv2/opencv.hpp>

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

  // 1. 预生成 UUID
  auto& l_set    = core_set::get_set();
  auto l_preview = std::make_shared<sd2::ai_preview_file>();
  using namespace orm;
  l_preview->extension_         = l_ext;
  auto l_install_1              = insert(l_sql).into<sd2::ai_preview_file>().values(*l_preview);

  auto l_ref                    = std::make_shared<sd2::ai_entity_reference_preview>();
  l_ref->ai_generate_entity_id_ = entity_id_;
  l_ref->preview_file_          = l_preview->uuid_id_;
  auto l_install_2              = insert(l_sql).into<sd2::ai_entity_reference_preview>().values(*l_ref);

  // 2. 一次提交两个 insert — 遵循 ai_episode.cpp 模式
  co_await l_sql.run_sql(l_install_1, l_install_2);

  // 3. 存储路径
  auto& l_ctx           = g_ctx().get<kitsu_ctx_t>();
  auto l_file_picture   = l_ctx.get_sd2_pictures_file(l_preview->uuid_id_, l_ext);
  auto l_file_thumbnail = l_ctx.get_sd2_thumbnail_file(l_preview->uuid_id_);

  if (auto l_p = l_file_picture.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  if (auto l_p = l_file_thumbnail.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);

  // 4. 生成缩略图（视频第一帧）
  {
    auto l_video = cv::VideoCapture{l_file.generic_string()};
    cv::Mat l_image{};
    l_video >> l_image;
    if (l_image.empty()) throw_exception(doodle_error{"视频解码失败"});

    auto l_resize = std::min(500.0 / l_image.cols, 500.0 / l_image.rows);
    cv::resize(l_image, l_image, cv::Size(l_image.cols * l_resize, l_image.rows * l_resize));
    cv::imwrite(l_file_thumbnail.generic_string(), l_image);
  }

  // 5. 保存原始视频到最终路径
  FSys::rename(l_file, l_file_picture);

  co_return in_handle->make_msg(nlohmann::json{{"reference", *l_ref}, {"preview", *l_preview}});
}

}  // namespace doodle::http::seedance2