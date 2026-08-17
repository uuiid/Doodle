#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/seedance2/ai_generate_entity.h>
#include <doodle_core/metadata/seedance2/ai_preview_file.h>

#include <doodle_lib/http_method/seedance2/reg.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "core/global_function.h"
#include "reg.h"
#include <memory>
#include <opencv2/opencv.hpp>

namespace doodle::http::seedance2 {
namespace sd2 = doodle::seedance2;

// /api/seedance2/subproject/{subproject_id}/entity
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_generate_entity, post) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  auto l_entity            = std::make_shared<sd2::ai_generate_entity>();
  l_json.get_to(*l_entity);

  co_await l_sql.install(l_entity);

  co_return in_handle->make_msg(nlohmann::json{} = *l_entity);
}

// /api/seedance2/subproject/{subproject_id}/entity/{entity_id}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_generate_entity_instance, get) {
  auto l_sql    = get_sqlite_database();
  auto l_entity = l_sql.get_by_uuid<sd2::ai_generate_entity>(entity_id_);

  co_return in_handle->make_msg(nlohmann::json{} = l_entity);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_generate_entity_instance, put) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  auto l_entity = std::make_shared<sd2::ai_generate_entity>(
      l_sql.get_by_uuid<sd2::ai_generate_entity>(entity_id_)
  );
  if (l_json.contains("name")) l_json.at("name").get_to(l_entity->name_);
  if (l_json.contains("ai_generate_classification_id")) l_json.at("ai_generate_classification_id").get_to(l_entity->ai_generate_classification_id_);
  if (l_json.contains("shot_uuid_id")) l_json.at("shot_uuid_id").get_to(l_entity->shot_uuid_id_);
  if (l_json.contains("preview_file")) l_json.at("preview_file").get_to(l_entity->preview_file_);

  co_await l_sql.update(l_entity);

  co_return in_handle->make_msg(nlohmann::json{} = *l_entity);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_generate_entity_instance, delete_) {
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();
  co_await l_sql.remove<sd2::ai_generate_entity>(entity_id_);

  co_return in_handle->make_msg(nlohmann::json{{"entity_id", entity_id_}});
}

// /api/seedance2/subproject/{subproject_id}/entity/{entity_id}/preview
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_entity_preview, post) {
  auto l_sql    = get_sqlite_database();
  auto l_entity = std::make_shared<sd2::ai_generate_entity>(l_sql.get_by_uuid<sd2::ai_generate_entity>(entity_id_));
  auto l_file   = in_handle->get_file();
  auto l_preview        = std::make_shared<sd2::ai_preview_file>();
  l_preview->extension_ = ".png";
  co_await l_sql.install(l_preview);
  l_entity->preview_file_ = l_preview->uuid_id_;
  co_await l_sql.update(l_entity);

  auto& l_ctx           = g_ctx().get<kitsu_ctx_t>();
  auto l_file_picture   = l_ctx.get_sd2_pictures_file(l_preview->uuid_id_, l_file.extension().string());
  auto l_file_thumbnail = l_ctx.get_sd2_thumbnail_file(l_preview->uuid_id_);

  if (auto l_p = l_file_picture.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  if (auto l_p = l_file_thumbnail.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);

  {
    auto l_image = cv::imread(l_file.generic_string());
    if (l_image.empty()) throw_exception(doodle_error{"图片解码失败"});
    auto l_resize = std::min(500.0 / l_image.cols, 500.0 / l_image.rows);
    cv::resize(l_image, l_image, cv::Size(l_image.cols * l_resize, l_image.rows * l_resize));
    cv::imwrite(l_file_thumbnail.generic_string(), l_image);
  }
  FSys::rename(l_file, l_file_picture);

  co_return in_handle->make_msg(nlohmann::json{{"id", l_preview->uuid_id_}});
}

}  // namespace doodle::http::seedance2