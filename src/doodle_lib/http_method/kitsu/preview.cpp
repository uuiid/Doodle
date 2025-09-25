//
// Created by TD on 25-3-27.
//

#include "doodle_core/metadata/comment.h"
#include "doodle_core/metadata/entity.h"
#include "doodle_core/metadata/preview_file.h"
#include "doodle_core/metadata/task.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"

#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu.h>

#include "kitsu_reg_url.h"
#include <opencv2/opencv.hpp>

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> actions_tasks_comments_add_preview::post(
    session_data_ptr in_handle
) {
  person_.check_task_action_access(task_id_);
  auto l_sql     = g_ctx().get<sqlite_database>();

  auto l_comment = l_sql.get_by_uuid<comment>(comment_id_);
  auto l_task    = l_sql.get_by_uuid<task>(task_id_);
  default_logger_raw()->info(
      "person {} actions_tasks_comments_add_preview {} {}", person_.person_.uuid_id_, task_id_, comment_id_
  );
  auto l_revision = in_handle->get_json().value("revision", 0);
  if (l_revision == 0 && !l_sql.has_preview_file(comment_id_))
    l_revision = l_sql.get_next_preview_revision(task_id_);
  else if (l_revision == 0)
    l_revision = l_sql.get_preview_revision(comment_id_);
  auto l_position                  = l_sql.get_next_position(task_id_, l_revision);

  auto l_preview_file              = std::make_shared<preview_file>();
  l_preview_file->uuid_id_         = core_set::get_set().get_uuid();
  l_preview_file->revision_        = l_revision;
  l_preview_file->task_id_         = task_id_;
  l_preview_file->person_id_       = person_.person_.uuid_id_;
  l_preview_file->position_        = l_position;
  l_preview_file->name_            = fmt::to_string(l_preview_file->uuid_id_).substr(0, 13);
  l_preview_file->status_          = preview_file_statuses::processing;
  l_preview_file->source_          = "webgui";
  l_preview_file->extension_       = "mp4";
  l_preview_file->created_at_      = chrono::system_clock::now();
  l_preview_file->updated_at_      = chrono::system_clock::now();
  auto l_preview_link              = std::make_shared<comment_preview_link>();
  l_preview_link->comment_id_      = comment_id_;
  l_preview_link->preview_file_id_ = l_preview_file->uuid_id_;

  co_await l_sql.install(l_preview_file);
  co_await l_sql.install(l_preview_link);
  // 产生事件( "preview-file:new", "comment:update")
  socket_io::broadcast(
      "preview-file:new",
      nlohmann::json{
          {"preview_file_id", l_preview_file->uuid_id_}, {"comment_id", comment_id_}, {"project_id", l_task.project_id_}
      },
      "/events"
  );
  socket_io::broadcast(
      "comment:update", nlohmann::json{{"comment_id", comment_id_}, {"project_id", l_task.project_id_}}, "/events"
  );

  co_return in_handle->make_msg(nlohmann::json{} = *l_preview_file);
}

namespace {
/// 图片扩展名
bool is_image_extension(const FSys::path& in_ext) {
  static const std::set<FSys::path> l_image_exts{".png", ".jpg", ".jpeg"};
  return l_image_exts.contains(in_ext);
}
/// 视频扩展名
bool is_video_extension(const FSys::path& in_ext) {
  static const std::set<FSys::path> l_video_exts{".mp4", ".avi", ".m4v", ".mov", ".webm"};
  return l_video_exts.contains(in_ext);
}
/// 将图片转换为png图片
FSys::path convert_to_png(const FSys::path& in_path) {
  auto l_new_path = in_path;
  if (l_new_path.extension() != ".png") {
    l_new_path.replace_extension(".png");
    auto l_cv = cv::imread(in_path.generic_string());
    if (l_cv.empty()) {
      throw_exception(doodle_error{"无法读取图片文件: " + in_path.generic_string()});
    }
    cv::imwrite(l_new_path.generic_string(), l_cv);
  }
  return l_new_path;
}
std::tuple<std::size_t, std::size_t> get_image_size(const FSys::path& in_path) {
  auto l_cv = cv::imread(in_path.generic_string());
  if (l_cv.empty()) {
    throw_exception(doodle_error{"无法读取图片文件: " + in_path.generic_string()});
  }
  return {l_cv.cols, l_cv.rows};
}
cv::Size save_variants(const cv::Mat& in_image, const uuid& in_id);
cv::Size save_variants(const FSys::path& in_path, const uuid& in_id) {
  return save_variants(cv::imread(in_path.generic_string()), in_id);
}
cv::Size save_variants(const cv::Mat& in_image, const uuid& in_id) {
  static std::array g_variants{
      std::tuple{"thumbnails", std::make_pair(150, 100)}, std::tuple{"thumbnails_square", std::make_pair(100, 100)},
      std::tuple{"previews", std::make_pair(1200, 0)}
  };
  cv::Size l_size = in_image.size();
  for (auto&& [key, size] : g_variants) {
    auto l_new_path =
        g_ctx().get<kitsu_ctx_t>().root_ / "pictures" / key / FSys::split_uuid_path(fmt::format("{}.png", in_id));
    if (auto l_p = l_new_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
    auto l_new_cv = in_image.clone();
    auto l_size_  = cv::Size{
        size.first, size.second == 0
                         ? boost::numeric_cast<std::int32_t>(
                              boost::numeric_cast<double>(in_image.rows) / boost::numeric_cast<double>(in_image.cols) *
                              boost::numeric_cast<double>(size.first)
                          )
                         : std::int32_t{size.second}
    };
    cv::resize(l_new_cv, l_new_cv, l_size_, 0, 0);
    cv::imwrite(l_new_path.generic_string(), l_new_cv);
  }
  return l_size;
}
/// 创建视频平铺图像
auto create_video_tile_image(cv::VideoCapture& in_capture, const cv::Size& in_size) {
  std::double_t l_frame_count = in_capture.get(cv::CAP_PROP_FRAME_COUNT);
  auto l_rows                 = std::min(480, boost::numeric_cast<std::int32_t>(std::ceil(l_frame_count / 8)));
  std::int32_t l_cols{8};
  // 确认步进大小
  std::double_t l_step = l_frame_count > l_rows * l_cols ? l_frame_count / l_rows / l_cols : 1.0;
  std::int32_t l_height{100}, l_width{boost::numeric_cast<std::int32_t>(in_size.aspectRatio() * 100)};
  cv::Mat l_tiles = cv::Mat::zeros(l_rows * l_height, l_cols * l_width, CV_8UC3);
  cv::Mat l_frame{};
  for (std::double_t l_i = 0; l_i < l_frame_count; l_i += l_step) {
    in_capture.set(cv::CAP_PROP_POS_FRAMES, std::floor(l_i));
    std::int32_t l_row{boost::numeric_cast<std::int32_t>(std::floor(l_i / l_cols))},
        l_col{boost::numeric_cast<std::int32_t>(std::floor(l_i)) % l_cols};

    if (in_capture.read(l_frame)) {
      cv::resize(l_frame, l_frame, cv::Size{l_width, l_height}, 0, 0);
      l_frame.copyTo(l_tiles(cv::Rect{l_col * l_width, l_row * l_height, l_width, l_height}));
    }
  }
  return l_tiles;
}

/// 处理上传的视频文件 格式化大小, 生成预览文件
auto handle_video_file(
    const FSys::path& in_path, const uuid& in_id, const std::size_t& in_fps, const cv::Size& in_size
) {
  auto l_low_file_path =
      g_ctx().get<kitsu_ctx_t>().root_ / "movies" / "lowdef" / FSys::split_uuid_path(fmt::format("{}.mp4", in_id));
  auto l_high_file_path =
      g_ctx().get<kitsu_ctx_t>().root_ / "movies" / "previews" / FSys::split_uuid_path(fmt::format("{}.mp4", in_id));
  if (auto l_p = l_low_file_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  if (auto l_p = l_high_file_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);

  auto l_video         = cv::VideoCapture{in_path.generic_string()};
  cv::Size l_high_size = in_size;
  cv::Size l_low_size{
      1280,
      boost::numeric_cast<std::int32_t>(std::floor(
          boost::numeric_cast<std::double_t>(in_size.height) / boost::numeric_cast<std::double_t>(in_size.width) * 1280
      ))
  };
  // 获取持续时间(秒)
  auto l_duration = l_video.get(cv::CAP_PROP_FRAME_COUNT) / l_video.get(cv::CAP_PROP_FPS);
  cv::Mat l_frame{};
  {
    auto l_low_vc = cv::VideoWriter{
        l_low_file_path.generic_string(), cv::VideoWriter::fourcc('a', 'v', 'c', '1'),
        boost::numeric_cast<std::double_t>(in_fps), l_low_size
    };
    auto l_high_vc = cv::VideoWriter{
        l_high_file_path.generic_string(), cv::VideoWriter::fourcc('a', 'v', 'c', '1'),
        boost::numeric_cast<std::double_t>(in_fps), l_high_size
    };
    if (!l_high_vc.isOpened() || !l_low_vc.isOpened())
      throw_exception(doodle_error{"无法创建视频文件: {} ", in_path.generic_string()});
    while (l_video.read(l_frame)) {
      if (l_frame.empty()) throw_exception(doodle_error{"无法读取视频文件: {} ", in_path.generic_string()});
      if (l_frame.cols != l_high_size.width || l_frame.rows != l_high_size.height) {
        cv::resize(l_frame, l_frame, l_high_size);
      }
      l_high_vc << l_frame;

      cv::resize(l_frame, l_frame, l_low_size);
      l_low_vc << l_frame;
    }
  }

  // 读取第一帧生成预览文件
  l_video.set(cv::CAP_PROP_POS_FRAMES, 0);
  l_video >> l_frame;
  if (l_frame.empty()) throw_exception(doodle_error{"无法读取视频文件: {} ", in_path.generic_string()});
  save_variants(l_frame, in_id);
  auto l_tiles = create_video_tile_image(l_video, l_high_size);
  auto l_path =
      g_ctx().get<kitsu_ctx_t>().root_ / "pictures" / "tiles" / FSys::split_uuid_path(fmt::format("{}.png", in_id));
  if (auto l_p = l_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  cv::imwrite(l_path.generic_string(), l_tiles);

  return std::make_tuple(l_high_size, l_duration, l_high_file_path);
}

}  // namespace

boost::asio::awaitable<boost::beast::http::message_generator> pictures_preview_files::post(session_data_ptr in_handle) {
  auto l_sql          = g_ctx().get<sqlite_database>();
  auto l_preview_file = std::make_shared<preview_file>(l_sql.get_by_uuid<preview_file>(id_));
  if (!l_preview_file->original_name_.empty())
    throw_exception(
        http_request_error{
            boost::beast::http::status::bad_request, "不允许重复上传预览文件, 请删除原有预览文件后再上传新的预览文件"
        }
    );
  person_.check_task_action_access(l_preview_file->task_id_);
  FSys::path l_file;
  if (auto l_fs = in_handle->get_files(); l_fs.empty())
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "请上传预览文件"});
  else
    l_file = l_fs[0];
  if (auto l_ext = l_file.extension(); is_image_extension(l_ext)) {
    auto l_size     = save_variants(l_file, l_preview_file->uuid_id_);
    /// 将原始文件添加到相应的位置中
    auto l_new_path = g_ctx().get<kitsu_ctx_t>().root_ / "pictures" / "original" /
                      FSys::split_uuid_path(fmt::format("{}.png", l_preview_file->uuid_id_));
    if (auto l_p = l_new_path.parent_path(); !exists(l_p)) FSys::create_directories(l_p);
    FSys::rename(l_file, l_new_path);
    l_preview_file->extension_     = "png";
    l_preview_file->original_name_ = l_file.stem().generic_string();
    l_preview_file->width_         = l_size.width;
    l_preview_file->height_        = l_size.height;
    l_file                         = l_new_path;
  } else if (is_video_extension(l_ext)) {
    auto l_new_path = g_ctx().get<kitsu_ctx_t>().root_ / "movies" / "source" /
                      FSys::split_uuid_path(fmt::format("{}.mp4", l_preview_file->uuid_id_));
    if (auto l_p = l_new_path.parent_path(); !exists(l_p)) FSys::create_directories(l_p);
    FSys::rename(l_file, l_new_path);
    auto l_task                              = l_sql.get_by_uuid<task>(l_preview_file->task_id_);
    auto l_prj                               = l_sql.get_by_uuid<project>(l_task.project_id_);
    auto l_prj_size                          = l_prj.get_resolution();

    auto&& [l_size, l_duration, l_high_file] = handle_video_file(
        l_new_path, l_preview_file->uuid_id_, l_prj.fps_, cv::Size{l_prj_size.first, l_prj_size.second}
    );

    l_file                         = l_high_file;

    l_preview_file->extension_     = "mp4";
    l_preview_file->original_name_ = l_file.stem().generic_string();
    l_preview_file->width_         = l_size.width;
    l_preview_file->height_        = l_size.height;
    l_preview_file->duration_      = l_duration;
  } else
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "不支持的预览文件格式"});

  l_preview_file->status_     = preview_file_statuses::ready;
  l_preview_file->file_size_  = FSys::file_size(l_file);
  l_preview_file->updated_at_ = chrono::system_clock::now();
  co_await l_sql.install(l_preview_file);

  // 更新task
  if (l_preview_file->position_ == 1) {
    auto l_task                   = std::make_shared<task>(l_sql.get_by_uuid<task>(l_preview_file->task_id_));
    l_task->last_preview_file_id_ = l_preview_file->uuid_id_;
    co_await l_sql.install(l_task);
    if (auto l_prj = l_sql.get_by_uuid<project>(l_task->project_id_); l_prj.is_set_preview_automated_) {
      auto l_entity              = std::make_shared<entity>(l_sql.get_by_uuid<entity>(l_task->entity_id_));
      l_entity->preview_file_id_ = l_preview_file->uuid_id_;
      // 发送事件 "preview-file:set-main"
      co_await l_sql.install(l_entity);
    }
  }
  co_return in_handle->make_msg(nlohmann::json{} = *l_preview_file);
}

boost::asio::awaitable<boost::beast::http::message_generator> actions_tasks_comments_preview_files::post(
    session_data_ptr in_handle
) {
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_task = std::make_shared<task>(l_sql.get_by_uuid<task>(task_id_));
  person_.check_task_action_access(*l_task);
  auto l_comment          = std::make_shared<comment>(l_sql.get_by_uuid<comment>(comment_id_));
  auto l_preview_file_    = std::make_shared<preview_file>(l_sql.get_by_uuid<preview_file>(preview_file_id_));

  std::int32_t l_revision = l_preview_file_->revision_;
  if (l_revision == 0 && !l_sql.has_preview_file(comment_id_))
    l_revision = l_sql.get_next_preview_revision(task_id_);
  else if (l_revision == 0)
    l_revision = l_sql.get_preview_revision(comment_id_);
  auto l_position                  = l_sql.get_next_position(task_id_, l_revision);

  auto l_preview_file              = std::make_shared<preview_file>();
  l_preview_file->uuid_id_         = core_set::get_set().get_uuid();
  l_preview_file->revision_        = l_revision;
  l_preview_file->task_id_         = task_id_;
  l_preview_file->person_id_       = person_.person_.uuid_id_;
  l_preview_file->position_        = l_position;
  l_preview_file->name_            = fmt::to_string(l_preview_file->uuid_id_).substr(0, 13);
  l_preview_file->status_          = preview_file_statuses::processing;
  l_preview_file->source_          = "webgui";
  l_preview_file->extension_       = "mp4";
  l_preview_file->created_at_      = chrono::system_clock::now();
  l_preview_file->updated_at_      = chrono::system_clock::now();
  auto l_preview_link              = std::make_shared<comment_preview_link>();
  l_preview_link->comment_id_      = comment_id_;
  l_preview_link->preview_file_id_ = l_preview_file->uuid_id_;

  co_await l_sql.install(l_preview_file);
  co_await l_sql.install(l_preview_link);
  socket_io::broadcast(
      "preview-file:new",
      nlohmann::json{
          {"preview_file_id", l_preview_file->uuid_id_},
          {"comment_id", comment_id_},
          {"project_id", l_task->project_id_}
      },
      "/events"
  );
  socket_io::broadcast(
      "comment:update", nlohmann::json{{"comment_id", comment_id_}, {"project_id", l_task->project_id_}}, "/events"
  );
  co_return in_handle->make_msg(nlohmann::json{} = *l_preview_file);
}

boost::asio::awaitable<boost::beast::http::message_generator> actions_preview_files_set_main_preview::put(
    session_data_ptr in_handle
) {
  auto l_sql          = g_ctx().get<sqlite_database>();
  auto l_preview_file = l_sql.get_by_uuid<preview_file>(id_);

  std::int32_t l_frame_number;
  if (auto l_json = in_handle->get_json(); l_json.contains("frame_number") && l_json["frame_number"].is_number()) {
    l_frame_number = l_json["frame_number"].get<std::int32_t>();
  }
  auto l_task = l_sql.get_by_uuid<task>(l_preview_file.task_id_);
  auto l_ent  = std::make_shared<entity>(l_sql.get_by_uuid<entity>(l_task.entity_id_));
  if (l_preview_file.extension_ == "mp4") {
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "mp4文件不支持设置为主预览文件"});
  } else {
    l_ent->preview_file_id_ = l_preview_file.uuid_id_;
    co_await l_sql.install(l_ent);
    // 发送事件 "preview-file:set-main"
  }
  co_return in_handle->make_msg(nlohmann::json{} = *l_ent);
}

}  // namespace doodle::http
