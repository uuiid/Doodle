#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/seedance2/ai_generate_entity.h>
#include <doodle_core/metadata/seedance2/ai_preview_file.h>
#include <doodle_core/metadata/seedance2/task.h>

#include <doodle_lib/http_method/seedance2/reg.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "core/global_function.h"
#include "reg.h"
#include <algorithm>
#include <map>
#include <memory>
#include <opencv2/opencv.hpp>

namespace doodle::http {
namespace sd2 = doodle::seedance2;

// /api/seedance2/subproject/{subproject_id}/episodes/{episode_id}/entity
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_entity, post) {
  person_.check_subproject_access(subproject_id_);

  person_.check_not_outsourcer();
  auto l_sql    = get_sqlite_database();
  auto l_json   = in_handle->get_json();

  auto l_entity = std::make_shared<sd2::ai_generate_entity>();
  l_json.get_to(*l_entity);

  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(insert(l_sql).into<sd2::ai_generate_entity>().values(*l_entity));
  co_await l_sql.run_sql(std::move(l_sqls));

  co_return std::make_unique<http_body_json_t>((nlohmann::json{} = *l_entity));
}
struct ai_generate_entity_with_preview_file : public sd2::ai_generate_entity {
  sd2::ai_preview_file preview_;
  std::vector<sd2::ai_generate_entity_sub_task> sub_task_list_;
  explicit ai_generate_entity_with_preview_file(
      const sd2::ai_generate_entity& in_reference, const sd2::ai_preview_file& in_preview
  )
      : sd2::ai_generate_entity(in_reference), preview_(in_preview) {}
  // to  json
  friend void to_json(nlohmann::json& j, const ai_generate_entity_with_preview_file& p) {
    to_json(j, static_cast<const sd2::ai_generate_entity&>(p));
    if (!p.preview_.uuid_id_.is_nil()) j["preview"] = p.preview_;
    j["sub_task_list"] = p.sub_task_list_;
  }
};
namespace {

/// 将不足4秒的视频补齐到4秒，使用最后一帧重复填充
/// @returns 指向（可能已补齐的）视频的 VideoCapture，已就绪可读取第一帧
cv::Mat pad_video_to_4s(const FSys::path& in_path) {
  auto l_fps         = 0.0;
  auto l_frame_count = 0.0;
  auto l_width       = 0;
  auto l_height      = 0;
  cv::Mat l_last_frame;
  int l_needed_frames = 0;
  FSys::path l_temp_path;
  bool l_padded = false;
  cv::Mat l_image{};

  {
    cv::VideoCapture l_capture{in_path.generic_string()};
    if (!l_capture.isOpened()) throw_exception(doodle_error{"视频打开失败"});
    l_capture >> l_image;

    l_fps         = l_capture.get(cv::CAP_PROP_FPS);
    l_frame_count = l_capture.get(cv::CAP_PROP_FRAME_COUNT);
    auto l_count  = static_cast<int>(l_frame_count);

    if (l_fps > 0.0 && l_count > 0 && (l_frame_count / l_fps) < 4.0) {
      l_width  = static_cast<int>(l_capture.get(cv::CAP_PROP_FRAME_WIDTH));
      l_height = static_cast<int>(l_capture.get(cv::CAP_PROP_FRAME_HEIGHT));

      l_capture.set(cv::CAP_PROP_POS_FRAMES, l_count - 1);
      l_capture >> l_last_frame;

      if (!l_last_frame.empty()) {
        auto l_target_frames = static_cast<int>(4.0 * l_fps + 0.999999);
        l_needed_frames      = l_target_frames - l_count;

        if (l_needed_frames > 0) {
          l_temp_path = in_path;
          l_temp_path.replace_extension(".padded_tmp.mp4");

          {
            auto l_writer = cv::VideoWriter{
                l_temp_path.generic_string(), cv::VideoWriter::fourcc('a', 'v', 'c', '1'), l_fps,
                cv::Size{l_width, l_height}
            };
            l_writer.set(cv::VIDEOWRITER_PROP_QUALITY, 100);

            l_capture.set(cv::CAP_PROP_POS_FRAMES, 0);
            cv::Mat l_frame;
            while (l_capture.read(l_frame)) {
              l_writer << l_frame;
            }
            for (int i = 0; i < l_needed_frames; ++i) {
              l_writer << l_last_frame;
            }
          }  // l_writer RAII 析构
          l_padded = true;
        }
      }
    }
  }  // l_capture RAII 析构，释放文件句柄

  if (l_padded) {
    FSys::remove(in_path);
    FSys::rename(l_temp_path, in_path);
  }

  return l_image;
}

void pad_video_to_8n1_frames(const FSys::path& in_path, const FSys::path& in_out_path) {
  DOODLE_CHICK(!in_path.empty() && FSys::exists(in_path), "视频文件不存在: {}", in_path.generic_string());
  DOODLE_CHICK(!in_out_path.empty(), "视频输出路径为空");

  if (FSys::exists(in_out_path) && FSys::equivalent(in_path, in_out_path)) return;

  auto l_capture = cv::VideoCapture{in_path.generic_string()};
  DOODLE_CHICK(l_capture.isOpened(), "视频打开失败: {}", in_path.generic_string());

  const auto l_fps     = l_capture.get(cv::CAP_PROP_FPS);
  const auto l_count   = static_cast<int>(l_capture.get(cv::CAP_PROP_FRAME_COUNT));
  DOODLE_CHICK(l_count > 0, "视频帧数读取失败: {}", in_path.generic_string());

  if (auto l_p = in_out_path.parent_path(); !l_p.empty() && !FSys::exists(l_p)) FSys::create_directories(l_p);

  if (l_count % 8 == 1) {
    FSys::copy_file(in_path, in_out_path, FSys::copy_options::overwrite_existing);
    return;
  }

  const auto l_width  = static_cast<int>(l_capture.get(cv::CAP_PROP_FRAME_WIDTH));
  const auto l_height = static_cast<int>(l_capture.get(cv::CAP_PROP_FRAME_HEIGHT));
  DOODLE_CHICK(l_fps > 0.0 && l_width > 0 && l_height > 0, "视频属性读取失败: {}", in_path.generic_string());

  cv::Mat l_last_frame{};
  for (auto l_pos = l_count - 1; l_pos >= 0 && l_last_frame.empty(); --l_pos) {
    l_capture.set(cv::CAP_PROP_POS_FRAMES, l_pos);
    l_capture >> l_last_frame;
  }
  DOODLE_CHICK(!l_last_frame.empty(), "视频解码失败: {}", in_path.generic_string());

  const auto l_target_frames = 8 * ((l_count - 1) / 8) + 9;
  const auto l_needed_frames = l_target_frames - l_count;

  auto l_writer = cv::VideoWriter{
      in_out_path.generic_string(), cv::VideoWriter::fourcc('a', 'v', 'c', '1'), l_fps,
      cv::Size{l_width, l_height}
  };
  l_writer.set(cv::VIDEOWRITER_PROP_QUALITY, 100);
  DOODLE_CHICK(l_writer.isOpened(), "视频写出失败: {}", in_out_path.generic_string());

  l_capture.set(cv::CAP_PROP_POS_FRAMES, 0);
  cv::Mat l_frame{};
  while (l_capture.read(l_frame)) l_writer << l_frame;
  for (auto l_i = 0; l_i < l_needed_frames; ++l_i) l_writer << l_last_frame;

  l_writer.release();
}

}  // namespace
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
                      .where(c(&sd2::ai_generate_entity::ai_episode_id_) == episode_id_)
                      .order_by(&sd2::ai_generate_entity::name_)()
                      .to_vector<ai_generate_entity_with_preview_file>();
  std::vector<uuid> l_entity_ids{};
  l_entity_ids.reserve(l_result.size());
  for (const auto& l_entity : l_result) l_entity_ids.emplace_back(l_entity.uuid_id_);

  if (!l_entity_ids.empty()) {
    auto l_sub_task_list = select(l_sql)
                               .columns(object<sd2::ai_generate_entity_sub_task>())
                               .from<sd2::ai_generate_entity_sub_task>()
                               .where(c(&sd2::ai_generate_entity_sub_task::ai_generate_entity_id_).in(l_entity_ids))()
                               .to_vector();
    std::map<uuid, std::vector<sd2::ai_generate_entity_sub_task>> l_sub_task_map{};
    for (auto& l_sub_task : l_sub_task_list) l_sub_task_map[l_sub_task.ai_generate_entity_id_].emplace_back(l_sub_task);
    for (auto& [l_key, l_value] : l_sub_task_map)
      std::ranges::sort(l_value, {}, &sd2::ai_generate_entity_sub_task::importance_);

    for (auto& l_entity : l_result) {
      if (auto l_iter = l_sub_task_map.find(l_entity.uuid_id_); l_iter != l_sub_task_map.end())
        l_entity.sub_task_list_ = l_iter->second;
    }
  }
  co_return std::make_unique<http_body_json_t>((nlohmann::json{} = l_result));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_generate_entity, put) {
  person_.check_subproject_access(subproject_id_);
  person_.check_not_outsourcer();

  auto l_sql = get_sqlite_database();
  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  auto l_update = update(l_sql)
                      .from<sd2::ai_generate_entity>()
                      .set(c(&sd2::ai_generate_entity::generate_count_) = 0)
                      .where(c(&sd2::ai_generate_entity::ai_episode_id_) == episode_id_);
  l_sqls.emplace_back(std::move(l_update));
  co_await l_sql.run_sql(std::move(l_sqls));

  co_return std::make_unique<http_body_json_t>(nlohmann::json{{"episode_id", episode_id_}});
}

// /api/seedance2/subproject/{subproject_id}/entity/{entity_id}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_generate_entity_instance, get) {
  person_.check_subproject_access(subproject_id_);

  auto l_sql    = get_sqlite_database();
  auto l_entity = l_sql.get_by_uuid<sd2::ai_generate_entity>(entity_id_);

  co_return std::make_unique<http_body_json_t>((nlohmann::json{} = l_entity));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_generate_entity_instance, put) {
  person_.check_subproject_access(subproject_id_);
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();
  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  auto l_update = update(l_sql).from<sd2::ai_generate_entity>().set_from_ref<sd2::ai_generate_entity>(l_json).where(
      c(&sd2::ai_generate_entity::uuid_id_) == entity_id_
  );
  l_sqls.emplace_back(std::move(l_update));
  co_await l_sql.run_sql(std::move(l_sqls));

  co_return std::make_unique<http_body_json_t>(
      (nlohmann::json{} = l_sql.get_by_uuid<sd2::ai_generate_entity>(entity_id_))
  );
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_generate_entity_instance, delete_) {
  person_.check_manager();
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();
  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(
      delete_from(l_sql).from<sd2::ai_generate_entity>().where(c(&sd2::ai_generate_entity::uuid_id_) == entity_id_)
  );
  co_await l_sql.run_sql(std::move(l_sqls));

  co_return std::make_unique<http_body_json_t>(nlohmann::json{{"entity_id", entity_id_}});
}

// /api/seedance2/subproject/{subproject_id}/entity/{entity_id}/preview
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_entity_preview, post) {
  person_.check_subproject_access(subproject_id_);
  auto l_sql            = get_sqlite_database();
  auto l_entity         = l_sql.get_by_uuid<sd2::ai_generate_entity>(entity_id_);
  auto l_file           = in_handle->get_file();
  auto l_preview        = std::make_shared<sd2::ai_preview_file>();
  l_preview->extension_ = ".png";
  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(insert(l_sql).into<sd2::ai_preview_file>().values(*l_preview));
  l_sqls.emplace_back(update(l_sql)
                          .from<sd2::ai_generate_entity>()
                          .set(c(&sd2::ai_generate_entity::preview_file_) = l_preview->uuid_id_)
                          .where(c(&sd2::ai_generate_entity::uuid_id_) == l_entity.uuid_id_));
  co_await l_sql.run_sql(std::move(l_sqls));

  auto& l_ctx           = g_kitsu_ctx();
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

  co_return std::make_unique<http_body_json_t>(nlohmann::json{{"id", l_preview->uuid_id_}});
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
  co_return std::make_unique<http_body_json_t>((nlohmann::json{} = l_result));
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
  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(insert(l_sql).into<sd2::ai_preview_file>().values(*l_preview));

  auto l_ref                    = std::make_shared<sd2::ai_entity_reference_preview>();
  l_ref->ai_generate_entity_id_ = entity_id_;
  l_ref->preview_file_          = l_preview->uuid_id_;
  l_sqls.emplace_back(insert(l_sql).into<sd2::ai_entity_reference_preview>().values(*l_ref));
  co_await l_sql.run_sql(std::move(l_sqls));

  auto& l_ctx           = g_kitsu_ctx();
  auto l_file_picture   = l_ctx.get_sd2_pictures_file(l_preview->uuid_id_, l_ext);
  auto l_file_thumbnail = l_ctx.get_sd2_thumbnail_file(l_preview->uuid_id_);

  if (auto l_p = l_file_picture.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);

  if (!l_is_audio) {
    if (auto l_p = l_file_thumbnail.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);

    cv::Mat l_image{};
    if (l_is_video) {
      l_image = pad_video_to_4s(l_file);
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

  co_return std::make_unique<http_body_json_t>(nlohmann::json{{"reference", *l_ref}, {"preview", *l_preview}});
}

// /api/seedance2/subproject/{subproject_id}/reference/{id}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_reference_instance, delete_) {
  person_.check_subproject_access(subproject_id_);
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();
  auto l_ref =
      std::make_shared<sd2::ai_entity_reference_preview>(l_sql.get_by_uuid<sd2::ai_entity_reference_preview>(id_));

  auto& l_ctx           = g_kitsu_ctx();
  auto l_preview        = l_sql.get_by_uuid<sd2::ai_preview_file>(l_ref->preview_file_);
  auto l_file_picture   = l_ctx.get_sd2_pictures_file(l_preview.uuid_id_, l_preview.extension_);
  auto l_file_thumbnail = l_ctx.get_sd2_thumbnail_file(l_preview.uuid_id_);

  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(delete_from(l_sql).from<sd2::ai_entity_reference_preview>().where(
      c(&sd2::ai_entity_reference_preview::uuid_id_) == id_
  ));
  l_sqls.emplace_back(
      delete_from(l_sql).from<sd2::ai_preview_file>().where(c(&sd2::ai_preview_file::uuid_id_) == l_ref->preview_file_)
  );
  co_await l_sql.run_sql(std::move(l_sqls));

  co_return std::make_unique<http_body_json_t>(nlohmann::json{{"id", id_}});
}

// /api/seedance2/subproject/{subproject_id}/entity/{entity_id}/task/{task_id}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_entity_sub_task, post) {
  person_.check_subproject_access(subproject_id_);
  person_.check_not_outsourcer();
  auto l_sql      = get_sqlite_database();
  auto l_sub_task = std::make_shared<sd2::ai_generate_entity_sub_task>(in_handle->get_json());

  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(insert(l_sql).into<sd2::ai_generate_entity_sub_task>().values(*l_sub_task));
  co_await l_sql.run_sql(std::move(l_sqls));

  co_return std::make_unique<http_body_json_t>((nlohmann::json{} = *l_sub_task));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_entity_sub_task, put) {
  person_.check_subproject_access(subproject_id_);
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();
  for (const auto& [l_key, l_value] : l_json.items())
    DOODLE_CHICK_HTTP(l_key == "importance", bad_request, "只支持修改 importance 字段: {}", l_key);
  DOODLE_CHICK_HTTP(l_json.contains("importance"), bad_request, "缺少 importance");

  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(
      update(l_sql)
          .from<sd2::ai_generate_entity_sub_task>()
          .set(c(&sd2::ai_generate_entity_sub_task::importance_) = l_json.at("importance").get<std::int64_t>())
          .where(
              c(&sd2::ai_generate_entity_sub_task::ai_generate_entity_id_) == entity_id_ &&
              c(&sd2::ai_generate_entity_sub_task::task_id_) == task_id_
          )
  );
  co_await l_sql.run_sql(std::move(l_sqls));

  auto l_sub_task_list =
      select(l_sql)
          .columns(object<sd2::ai_generate_entity_sub_task>())
          .from<sd2::ai_generate_entity_sub_task>()
          .where(c(&sd2::ai_generate_entity_sub_task::ai_generate_entity_id_) == entity_id_ && c(&sd2::ai_generate_entity_sub_task::task_id_) == task_id_)()
          .to_vector();
  DOODLE_CHICK_HTTP(!l_sub_task_list.empty(), bad_request, "次要任务不存在");
  co_return std::make_unique<http_body_json_t>((nlohmann::json{} = l_sub_task_list.front()));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_entity_sub_task, delete_) {
  person_.check_subproject_access(subproject_id_);
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();
  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(delete_from(l_sql).from<sd2::ai_generate_entity_sub_task>().where(
      c(&sd2::ai_generate_entity_sub_task::ai_generate_entity_id_) == entity_id_ &&
      c(&sd2::ai_generate_entity_sub_task::task_id_) == task_id_
  ));
  co_await l_sql.run_sql(std::move(l_sqls));

  co_return std::make_unique<http_body_json_t>(nlohmann::json{{"task_id", task_id_}});
}

}  // namespace doodle::http