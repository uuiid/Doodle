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

// /api/seedance2/subproject/{subproject_id}/episodes/{episode_id}/entity
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_entity, post) {
  person_.check_subproject_access(subproject_id_);

  person_.check_not_outsourcer();
  auto l_sql    = get_sqlite_database();
  auto l_json   = in_handle->get_json();

  auto l_entity = std::make_shared<sd2::ai_generate_entity>();
  l_json.get_to(*l_entity);

  co_await l_sql.install(l_entity);

  co_return in_handle->make_msg(nlohmann::json{} = *l_entity);
}
struct ai_generate_entity_with_preview_file : public sd2::ai_generate_entity {
  sd2::ai_preview_file preview_;
  explicit ai_generate_entity_with_preview_file(
      const sd2::ai_generate_entity& in_reference, const sd2::ai_preview_file& in_preview
  )
      : sd2::ai_generate_entity(in_reference), preview_(in_preview) {}
  // to  json
  friend void to_json(nlohmann::json& j, const ai_generate_entity_with_preview_file& p) {
    to_json(j, static_cast<const sd2::ai_generate_entity&>(p));
    if (!p.preview_.uuid_id_.is_nil()) j["preview"] = p.preview_;
  }
};
namespace {}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_generate_entity, get) {
  person_.check_subproject_access(subproject_id_);

  auto l_sql = get_sqlite_database();
  using namespace orm;
  auto l_result = select(l_sql)
                      .columns(object<sd2::ai_generate_entity>(), object<sd2::ai_preview_file>())
                      .from<sd2::ai_generate_entity>()
                      .left_outer_join<sd2::ai_preview_file>(
                          &sd2::ai_generate_entity::preview_file_, &sd2::ai_preview_file::uuid_id_
                      )
                      .where(c(&sd2::ai_generate_entity::ai_episode_id_) == episode_id_)()
                      .to_vector<ai_generate_entity_with_preview_file>();
  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}

// /api/seedance2/subproject/{subproject_id}/entity/{entity_id}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_generate_entity_instance, get) {
  person_.check_subproject_access(subproject_id_);

  auto l_sql    = get_sqlite_database();
  auto l_entity = l_sql.get_by_uuid<sd2::ai_generate_entity>(entity_id_);

  co_return in_handle->make_msg(nlohmann::json{} = l_entity);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_generate_entity_instance, put) {
  person_.check_subproject_access(subproject_id_);
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();
  using namespace orm;
  auto l_update = update(l_sql).from<sd2::ai_generate_entity>().set_from_ref<sd2::ai_generate_entity>(l_json).where(
      c(&sd2::ai_generate_entity::uuid_id_) == entity_id_
  );
  co_await l_sql.run_sql(l_update);

  co_return in_handle->make_msg(nlohmann::json{} = l_sql.get_by_uuid<sd2::ai_generate_entity>(entity_id_));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_generate_entity_instance, delete_) {
  person_.check_manager();
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();
  co_await l_sql.remove<sd2::ai_generate_entity>(entity_id_);

  co_return in_handle->make_msg(nlohmann::json{{"entity_id", entity_id_}});
}

// /api/seedance2/subproject/{subproject_id}/entity/{entity_id}/preview
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_entity_preview, post) {
  person_.check_subproject_access(subproject_id_);
  auto l_sql     = get_sqlite_database();
  auto l_entity  = l_sql.get_by_uuid<sd2::ai_generate_entity>(entity_id_);
  auto l_file    = in_handle->get_file();
  auto l_preview = std::make_shared<sd2::ai_preview_file>();
  l_preview->extension_ = ".png";
  co_await l_sql.install(l_preview);
  using namespace orm;
  co_await l_sql.run_sql(
      update(l_sql)
          .from<sd2::ai_generate_entity>()
          .set(c(&sd2::ai_generate_entity::preview_file_) = l_preview->uuid_id_)
          .where(c(&sd2::ai_generate_entity::uuid_id_) == l_entity.uuid_id_)
  );

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
namespace {
struct ai_entity_reference_preview_with_preview_file : public sd2::ai_entity_reference_preview {
  sd2::ai_preview_file preview_;
  explicit ai_entity_reference_preview_with_preview_file(
      const sd2::ai_entity_reference_preview& in_reference, const sd2::ai_preview_file& in_preview
  )
      : sd2::ai_entity_reference_preview(in_reference), preview_(in_preview) {}
  // to  json
  friend void to_json(nlohmann::json& j, const ai_entity_reference_preview_with_preview_file& p) {
    to_json(j, static_cast<const sd2::ai_entity_reference_preview>(p));
    j["preview"] = p.preview_;
  }
};

}  // namespace
// /api/seedance2/subproject/{subproject_id}/entity/{entity_id}/reference
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_entity_reference, get) {
  person_.check_subproject_access(subproject_id_);
  auto l_sql = get_sqlite_database();
  using namespace orm;
  auto l_result = select(l_sql)
                      .columns(object<sd2::ai_entity_reference_preview>(), object<sd2::ai_preview_file>())
                      .from<sd2::ai_entity_reference_preview>()
                      .join<sd2::ai_preview_file>(
                          c(&sd2::ai_entity_reference_preview::preview_file_) == c(&sd2::ai_preview_file::uuid_id_)
                      )
                      .where(c(&sd2::ai_entity_reference_preview::ai_generate_entity_id_) == entity_id_)()
                      .to_vector<ai_entity_reference_preview_with_preview_file>();
  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_entity_reference, post) {
  person_.check_subproject_access(subproject_id_);
  person_.check_not_outsourcer();
  auto l_sql      = get_sqlite_database();
  auto l_entity   = std::make_shared<sd2::ai_generate_entity>(l_sql.get_by_uuid<sd2::ai_generate_entity>(entity_id_));
  auto l_file     = in_handle->get_file();
  auto l_ext      = l_file.extension().string();
  auto l_is_video = l_ext == ".mp4" || l_ext == ".mov" || l_ext == ".avi";
  auto l_is_audio =
      l_ext == ".mp3" || l_ext == ".wav" || l_ext == ".ogg" || l_ext == ".aac" || l_ext == ".wma" || l_ext == ".m4a";

  auto l_preview        = std::make_shared<sd2::ai_preview_file>();
  l_preview->extension_ = l_ext;
  co_await l_sql.install(l_preview);

  auto l_ref                    = std::make_shared<sd2::ai_entity_reference_preview>();
  l_ref->ai_generate_entity_id_ = entity_id_;
  l_ref->preview_file_          = l_preview->uuid_id_;
  co_await l_sql.install(l_ref);

  auto& l_ctx           = g_ctx().get<kitsu_ctx_t>();
  auto l_file_picture   = l_ctx.get_sd2_pictures_file(l_preview->uuid_id_, l_ext);
  auto l_file_thumbnail = l_ctx.get_sd2_thumbnail_file(l_preview->uuid_id_);

  if (auto l_p = l_file_picture.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);

  if (!l_is_audio) {
    if (auto l_p = l_file_thumbnail.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);

    cv::Mat l_image{};
    if (l_is_video) {
      auto l_video = cv::VideoCapture{l_file.generic_string()};
      l_video >> l_image;
      if (l_image.empty()) throw_exception(doodle_error{"视频解码失败"});
    } else {
      l_image = cv::imread(l_file.generic_string());
      if (l_image.empty()) throw_exception(doodle_error{"图片解码失败"});
    }
    auto l_resize = std::min(500.0 / l_image.cols, 500.0 / l_image.rows);
    cv::resize(l_image, l_image, cv::Size(l_image.cols * l_resize, l_image.rows * l_resize));
    cv::imwrite(l_file_thumbnail.generic_string(), l_image);
  }

  FSys::rename(l_file, l_file_picture);

  co_return in_handle->make_msg(nlohmann::json{{"reference", *l_ref}, {"preview", *l_preview}});
}

// /api/seedance2/subproject/{subproject_id}/reference/{id}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_reference_instance, delete_) {
  person_.check_subproject_access(subproject_id_);
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();
  auto l_ref =
      std::make_shared<sd2::ai_entity_reference_preview>(l_sql.get_by_uuid<sd2::ai_entity_reference_preview>(id_));

  auto& l_ctx           = g_ctx().get<kitsu_ctx_t>();
  auto l_preview        = l_sql.get_by_uuid<sd2::ai_preview_file>(l_ref->preview_file_);
  auto l_file_picture   = l_ctx.get_sd2_pictures_file(l_preview.uuid_id_, l_preview.extension_);
  auto l_file_thumbnail = l_ctx.get_sd2_thumbnail_file(l_preview.uuid_id_);

  if (FSys::exists(l_file_picture)) FSys::remove(l_file_picture);
  if (FSys::exists(l_file_thumbnail)) FSys::remove(l_file_thumbnail);

  co_await l_sql.remove<sd2::ai_entity_reference_preview>(id_);
  co_await l_sql.remove<sd2::ai_preview_file>(l_ref->preview_file_);

  co_return in_handle->make_msg(nlohmann::json{{"id", id_}});
}

}  // namespace doodle::http::seedance2