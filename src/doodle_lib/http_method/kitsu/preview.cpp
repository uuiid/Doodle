//
// Created by TD on 25-3-27.
//

#include "doodle_core/core/core_set.h"
#include "doodle_core/core/file_sys.h"
#include "doodle_core/core/global_function.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/metadata/comment.h"
#include "doodle_core/metadata/entity.h"
#include "doodle_core/metadata/preview_file.h"
#include "doodle_core/metadata/task.h"
#include "doodle_core/metadata/task_type.h"
#include "doodle_core/sqlite_orm/detail/sqlite_database_impl.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"

#include "doodle_lib/core/http/http_function.h"
#include <doodle_lib/core/ffmpeg_video.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/preview.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/post.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include "kitsu_reg_url.h"
#include "long_task/image_to_move.h"
#include <algorithm>
#include <filesystem>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <opencv2/opencv.hpp>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <sqlite_orm/sqlite_orm.h>
#include <tuple>
#include <vector>

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> actions_tasks_comments_add_preview::post(
    session_data_ptr in_handle
) {
  person_.check_task_action_access(task_id_);
  auto l_sql     = g_ctx().get<sqlite_database>();

  auto l_comment = l_sql.get_by_uuid<comment>(comment_id_);
  auto l_task    = l_sql.get_by_uuid<task>(task_id_);
  auto l_revision = in_handle->get_json().value("revision", 0);
  if (l_revision == 0 && !l_sql.has_preview_file(comment_id_))
    l_revision = l_sql.get_next_preview_revision(task_id_);
  else if (l_revision == 0)
    l_revision = l_sql.get_preview_revision(comment_id_);
  auto l_position     = l_sql.get_next_position(task_id_, l_revision);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(),
      "用户 {}({}) 开始创建预览文件(评论关联) task_id {} comment_id {} revision {} position {}",
      person_.person_.email_, person_.person_.get_full_name(), task_id_, comment_id_, l_revision, l_position
  );

  auto l_preview_file = std::make_shared<preview_file>();
  in_handle->get_json().get_to(*l_preview_file);
  l_preview_file->revision_  = l_revision;
  l_preview_file->task_id_   = task_id_;
  l_preview_file->person_id_ = person_.person_.uuid_id_;
  l_preview_file->position_  = l_position;
  l_preview_file->name_      = core_set::get_set().get_uuid_str();
  l_preview_file->status_    = preview_file_statuses::processing;
  l_preview_file->extension_ = "mp4";
  co_await l_sql.install(l_preview_file);

  auto l_preview_link              = std::make_shared<comment_preview_link>();
  l_preview_link->comment_id_      = comment_id_;
  l_preview_link->preview_file_id_ = l_preview_file->uuid_id_;
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

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(),
      "用户 {}({}) 完成创建预览文件(评论关联) task_id {} comment_id {} preview_file_id {} revision {} position {}",
      person_.person_.email_, person_.person_.get_full_name(), task_id_, comment_id_, l_preview_file->uuid_id_,
      l_revision, l_position
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

struct handle_video_file_t {
  FSys::path video_path_;
  std::size_t fps_{0};
  cv::Size size_{0, 0};
  std::shared_ptr<preview_file> preview_file_;
};

cv::Size save_variants(const cv::Mat& in_image, const uuid& in_id) {
  DOODLE_CHICK(!in_image.empty(), "保存变体图片时输入图片为空");
  auto& l_ctx = g_ctx().get<kitsu_ctx_t>();
  auto l_now  = std::chrono::steady_clock::now();
  auto l_sql  = g_ctx().get<sqlite_database>();
  doodle::detail::add_watermark_t l_add_watermark{l_sql.get_all<organisation>().front().name_, 150};
  auto l_watermarked_image = l_add_watermark(in_image, {1920, 1080});
  std::array g_variants{
      std::tuple{l_ctx.get_pictures_thumbnails_file(in_id), std::make_pair(150, 100), in_image},
      std::tuple{l_ctx.get_pictures_thumbnails_square_file(in_id), std::make_pair(100, 100), in_image},
      std::tuple{l_ctx.get_pictures_preview_file(in_id), std::make_pair(1200, 0), in_image},
      std::tuple{l_ctx.get_outsource_pictures_preview_file(in_id), std::make_pair(1200, 0), l_watermarked_image}
  };
  cv::Size l_size = in_image.size();
  for (auto&& [l_path, size, l_image] : g_variants) {
    if (auto l_p = l_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
    auto l_new_cv = l_image.clone();
    auto l_size_  = cv::Size{
        size.first, size.second == 0
                         ? boost::numeric_cast<std::int32_t>(
                              boost::numeric_cast<double>(l_image.rows) / boost::numeric_cast<double>(l_image.cols) *
                              boost::numeric_cast<double>(size.first)
                          )
                         : std::int32_t{size.second}
    };
    cv::resize(l_new_cv, l_new_cv, l_size_, 0, 0);
    cv::imwrite(l_path.generic_string(), l_new_cv);
  }
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_long_task(), "保存图片变体完成, 时间 {:%H:%M:%S}", std::chrono::steady_clock::now() - l_now
  );
  return l_size;
}

cv::Size save_watermarked_image(const cv::Mat& in_image, const uuid& in_id) {
  DOODLE_CHICK(!in_image.empty(), "保存加水印图片时输入图片为空");

  auto& l_ctx = g_ctx().get<kitsu_ctx_t>();
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_path = l_ctx.get_outsource_pictures_original_file(in_id);
  if (auto l_p = l_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  doodle::detail::add_watermark_t l_add_watermark{l_sql.get_all<organisation>().front().name_, 150};
  auto l_cv = l_add_watermark(in_image, {1920, 1080});
  cv::imwrite(l_path.generic_string(), l_cv);
  return l_cv.size();
}

/// 创建视频平铺图像
auto create_video_tile_image(cv::VideoCapture& in_capture, const handle_video_file_t& in_files) {
  auto l_now = std::chrono::steady_clock::now();
  spdlog::warn("创建视频平铺图像, 目标尺寸 {}x{}", in_files.size_.width, in_files.size_.height);
  const std::double_t l_frame_count = in_capture.get(cv::CAP_PROP_FRAME_COUNT);
  const std::int32_t l_rows         = std::min(480, boost::numeric_cast<std::int32_t>(std::ceil(l_frame_count / 8)));
  const std::int32_t l_cols{8};
  // 确认步进大小
  const std::double_t l_step = l_frame_count > (l_rows * l_cols) ? l_frame_count / l_rows / l_cols : 1.0;
  const std::int32_t l_height{100}, l_width{boost::numeric_cast<std::int32_t>(in_files.size_.aspectRatio() * 100)};
  cv::Mat l_tiles = cv::Mat::zeros(l_rows * l_height, l_cols * l_width, CV_8UC3);
  cv::Mat l_frame{};
  const auto l_total_tiles = static_cast<std::int32_t>(l_rows * l_cols);
  // 生成瓦片数需要的帧
  std::set<std::int32_t> l_needed_frames{};
  for (std::int32_t l_i = 0; l_i < l_frame_count && l_i < l_total_tiles; ++l_i)
    l_needed_frames.insert(boost::numeric_cast<std::int32_t>(std::floor(l_i * l_step)));
  in_capture.set(cv::CAP_PROP_POS_FRAMES, 0);
  for (std::int32_t l_i = 0; l_i < l_frame_count && l_i < l_total_tiles; ++l_i) {
    if (in_capture.read(l_frame) == false || l_frame.empty()) {
      SPDLOG_LOGGER_WARN(
          g_logger_ctrl().get_long_task(), "读取视频 {} 帧失败, 当前帧 {}, 总帧数 {}", in_files.video_path_, l_i,
          l_frame_count
      );
      continue;
    }
    if (l_needed_frames.contains(l_i)) {
      std::int32_t l_row{l_i / l_cols}, l_col{l_i % l_cols};
      cv::resize(l_frame, l_frame, cv::Size{l_width, l_height}, 0, 0);
      l_frame.copyTo(l_tiles(cv::Rect{l_col * l_width, l_row * l_height, l_width, l_height}));
    }
  }
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_long_task(), "生成视频平铺图像完成, 行数 {}, 列数 {}, 时间 {:%H:%M:%S}", l_rows, l_cols,
      std::chrono::steady_clock::now() - l_now
  );
  return l_tiles;
}

}  // namespace
namespace preview {
video_info_t get_video_duration(const FSys::path& in_path) {
  auto l_video    = cv::VideoCapture{in_path.generic_string()};
  auto l_duration = l_video.get(cv::CAP_PROP_FRAME_COUNT) / l_video.get(cv::CAP_PROP_FPS);
  return video_info_t{
      l_duration, cv::Size{
                      static_cast<int>(l_video.get(cv::CAP_PROP_FRAME_WIDTH)),
                      static_cast<int>(l_video.get(cv::CAP_PROP_FRAME_HEIGHT))
                  }
  };
}

/// 处理上传的视频文件 格式化大小, 生成预览文件
std::tuple<cv::Size, double, FSys::path> handle_video_file(
    const FSys::path& in_path, const std::size_t& in_fps, const cv::Size& in_size,
    const std::shared_ptr<preview_file>& in_preview_file
) {
  handle_video_file_t l_files{in_path, in_fps, in_size, in_preview_file};

  auto l_low_file_path         = g_ctx().get<kitsu_ctx_t>().get_movie_lowdef_file(in_preview_file->uuid_id_);
  auto l_high_file_path        = g_ctx().get<kitsu_ctx_t>().get_movie_preview_file(in_preview_file->uuid_id_);
  auto l_low_file_path_backup  = FSys::add_time_stamp(l_low_file_path);
  auto l_high_file_path_backup = FSys::add_time_stamp(l_high_file_path);
  if (auto l_p = l_low_file_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  if (auto l_p = l_high_file_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);

  {
    ffmpeg_video_resize l_resizer{in_path, l_high_file_path_backup, l_low_file_path_backup, in_size};
    l_resizer.process();
  }

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
  // 读取第一帧生成预览文件
  l_video.set(cv::CAP_PROP_POS_FRAMES, 0);
  l_video >> l_frame;
  if (l_frame.empty()) throw_exception(doodle_error{"无法读取视频文件: {} ", in_path.generic_string()});
  save_variants(l_frame, in_preview_file->uuid_id_);
  auto l_tiles = create_video_tile_image(l_video, l_files);
  auto l_path  = g_ctx().get<kitsu_ctx_t>().root_ / "pictures" / "tiles" /
                FSys::split_uuid_path(fmt::format("{}.png", in_preview_file->uuid_id_));
  auto l_path_backup = FSys::add_time_stamp(l_path);
  if (auto l_p = l_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  cv::imwrite(l_path_backup.generic_string(), l_tiles);

  {  // rename
    FSys::rename(l_low_file_path_backup, l_low_file_path);
    FSys::rename(l_high_file_path_backup, l_high_file_path);
    FSys::rename(l_path_backup, l_path);
  }
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_long_task(), "生成视频 {} {}, 图片 {}", l_low_file_path, l_high_file_path, l_path
  );
  in_preview_file->file_size_ = FSys::exists(l_high_file_path) ? FSys::file_size(l_high_file_path) : 0;
  in_preview_file->status_    = preview_file_statuses::ready;
  boost::asio::co_spawn(g_io_context(), g_ctx().get<sqlite_database>().update(in_preview_file), boost::asio::detached);

  return std::make_tuple(l_high_size, l_duration, l_high_file_path);
}
}  // namespace preview
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
  if (auto l_fs = in_handle->get_file(); l_fs.empty())
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "请上传预览文件"});
  else
    l_file = l_fs;

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(),
      "用户 {}({}) 开始上传预览文件 preview_file_id {} task_id {} filename {} ext {}",
      person_.person_.email_, person_.person_.get_full_name(), l_preview_file->uuid_id_, l_preview_file->task_id_,
      l_file.filename().generic_string(), l_file.extension().generic_string()
  );
  if (auto l_ext = l_file.extension(); is_image_extension(l_ext)) {
    auto l_file_image = cv::imread(l_file.generic_string());
    auto l_size       = save_variants(l_file_image, l_preview_file->uuid_id_);
    save_watermarked_image(l_file_image, l_preview_file->uuid_id_);
    /// 将原始文件添加到相应的位置中
    auto l_new_path = g_ctx().get<kitsu_ctx_t>().get_pictures_original_file(l_preview_file->uuid_id_);
    if (auto l_p = l_new_path.parent_path(); !exists(l_p)) FSys::create_directories(l_p);
    FSys::rename(l_file, l_new_path);
    l_preview_file->extension_     = "png";
    l_preview_file->original_name_ = l_file.stem().generic_string();
    l_preview_file->width_         = l_size.width;
    l_preview_file->height_        = l_size.height;
    l_preview_file->status_        = preview_file_statuses::ready;
    l_file                         = l_new_path;
  } else if (is_video_extension(l_ext)) {
    auto l_new_path = g_ctx().get<kitsu_ctx_t>().get_movie_source_file(id_);
    if (auto l_p = l_new_path.parent_path(); !exists(l_p)) FSys::create_directories(l_p);
    FSys::rename(l_file, l_new_path);
    auto l_task     = l_sql.get_by_uuid<task>(l_preview_file->task_id_);
    auto l_prj      = l_sql.get_by_uuid<project>(l_task.project_id_);
    auto l_prj_size = l_prj.get_resolution();

    auto l_duration = preview::get_video_duration(l_new_path);
    boost::asio::post(
        g_pool_strand(),
        [l_new_path, fps = l_prj.fps_, l_preview_file, size = cv::Size{l_prj_size.first, l_prj_size.second}]() {
          preview::handle_video_file(l_new_path, fps, size, l_preview_file);
        }
    );

    l_file                         = g_ctx().get<kitsu_ctx_t>().get_movie_preview_file(l_preview_file->uuid_id_);
    l_preview_file->extension_     = "mp4";
    l_preview_file->original_name_ = l_file.stem().generic_string();
    l_preview_file->width_         = l_prj_size.first;
    l_preview_file->height_        = l_prj_size.second;
    l_preview_file->duration_      = l_duration.duration_;
    l_preview_file->status_        = preview_file_statuses::processing;
  } else
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "不支持的预览文件格式"});

  l_preview_file->file_size_ = FSys::exists(l_file) ? FSys::file_size(l_file) : 0;
  co_await l_sql.update(l_preview_file);

  // 更新task
  if (l_preview_file->position_ == 1) {
    auto l_task                   = std::make_shared<task>(l_sql.get_by_uuid<task>(l_preview_file->task_id_));
    l_task->last_preview_file_id_ = l_preview_file->uuid_id_;
    co_await l_sql.update(l_task);
    if (auto l_prj = l_sql.get_by_uuid<project>(l_task->project_id_); l_prj.is_set_preview_automated_) {
      auto l_entity              = std::make_shared<entity>(l_sql.get_by_uuid<entity>(l_task->entity_id_));
      l_entity->preview_file_id_ = l_preview_file->uuid_id_;
      // 发送事件 "preview-file:set-main"
      co_await l_sql.update(l_entity);
    }
  }

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(),
      "用户 {}({}) 完成上传预览文件 preview_file_id {} task_id {} status {} ext {} size {} width {} height {} duration {}",
      person_.person_.email_, person_.person_.get_full_name(), l_preview_file->uuid_id_, l_preview_file->task_id_,
      magic_enum::enum_name(l_preview_file->status_), l_preview_file->extension_, l_preview_file->file_size_,
      l_preview_file->width_, l_preview_file->height_, l_preview_file->duration_
  );
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
  auto l_position     = l_sql.get_next_position(task_id_, l_revision);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(),
      "用户 {}({}) 开始创建预览文件(从已有预览复制) task_id {} comment_id {} from_preview_file_id {} revision {} position {}",
      person_.person_.email_, person_.person_.get_full_name(), task_id_, comment_id_, preview_file_id_, l_revision,
      l_position
  );

  auto l_preview_file = std::make_shared<preview_file>();
  in_handle->get_json().get_to(*l_preview_file);
  l_preview_file->revision_  = l_revision;
  l_preview_file->task_id_   = task_id_;
  l_preview_file->person_id_ = person_.person_.uuid_id_;
  l_preview_file->position_  = l_position;
  l_preview_file->name_      = core_set::get_set().get_uuid_str();
  l_preview_file->status_    = preview_file_statuses::processing;
  l_preview_file->extension_ = "mp4";
  co_await l_sql.install(l_preview_file);

  auto l_preview_link              = std::make_shared<comment_preview_link>();
  l_preview_link->comment_id_      = comment_id_;
  l_preview_link->preview_file_id_ = l_preview_file->uuid_id_;
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

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(),
      "用户 {}({}) 完成创建预览文件(从已有预览复制) task_id {} comment_id {} preview_file_id {} revision {} position {}",
      person_.person_.email_, person_.person_.get_full_name(), task_id_, comment_id_, l_preview_file->uuid_id_,
      l_revision, l_position
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
    co_await l_sql.update(l_ent);
    // 发送事件 "preview-file:set-main"
  }
  co_return in_handle->make_msg(nlohmann::json{} = *l_ent);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_entities_preview_files, get) {
  auto l_sql = g_ctx().get<sqlite_database>();
  auto l_ent = l_sql.get_by_uuid<entity>(entity_id_);
  person_.check_in_project(l_ent.project_id_);
  person_.check_not_outsourcer();
  std::vector<preview_file> l_ret;
  using namespace sqlite_orm;
  l_ret = l_sql.impl_->storage_any_.get_all<preview_file>(
      join<task>(on(c(&preview_file::task_id_) == c(&task::uuid_id_))),
      join<task_type>(on(c(&task::task_type_id_) == c(&task_type::uuid_id_))),
      where(c(&task::entity_id_) == entity_id_),
      multi_order_by(
          order_by(&task_type::name_), order_by(&preview_file::revision_).desc(), order_by(&preview_file::position_)
      )
  );
  co_return in_handle->make_msg(nlohmann::json{} = l_ret);
}
struct data_fix_preview_files_thumbnails_run_t {
  std::shared_ptr<doodle::detail::add_watermark_t> watermark_adder_;
  void operator()() {
    auto l_sql      = g_ctx().get<sqlite_database>();
    auto l_previews = l_sql.impl_->storage_any_.get_all<preview_file>();
    watermark_adder_ =
        std::make_shared<doodle::detail::add_watermark_t>(l_sql.get_all<doodle::organisation>().front().name_, 150);
    auto& l_ctx       = g_ctx().get<kitsu_ctx_t>();
    const auto l_size = cv::Size{100, 100};
    SPDLOG_LOGGER_INFO(g_logger_ctrl().get_long_task(), "开始修正");
    for (auto&& l_preview : l_previews) {
      try {
        if (auto l_path = g_ctx().get<kitsu_ctx_t>().get_pictures_original_file(l_preview.uuid_id_),
            l_path2     = g_ctx().get<kitsu_ctx_t>().get_outsource_pictures_original_file(l_preview.uuid_id_);
            FSys::exists(l_path) && !FSys::exists(l_path2)) {
          if (auto l_p = l_path2.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
          (*watermark_adder_)(l_path, l_path2, {1920, 1080});
          SPDLOG_LOGGER_INFO(
              g_logger_ctrl().get_long_task(), "修复预览文件原始图片水印, preview_file_id {}", l_preview.uuid_id_
          );
        }
        if (auto l_path = g_ctx().get<kitsu_ctx_t>().get_pictures_preview_file(l_preview.uuid_id_),
            l_path2     = g_ctx().get<kitsu_ctx_t>().get_outsource_pictures_preview_file(l_preview.uuid_id_);
            FSys::exists(l_path) && !FSys::exists(l_path2)) {
          if (auto l_p = l_path2.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
          (*watermark_adder_)(l_path, l_path2, {1920, 1080});
          SPDLOG_LOGGER_INFO(
              g_logger_ctrl().get_long_task(), "修复预览文件预览图片水印, preview_file_id {}", l_preview.uuid_id_
          );
          auto l_target_path = l_ctx.get_pictures_thumbnails_square_file(l_preview.uuid_id_);
          if (!FSys::exists(l_target_path)) {
            auto l_image = cv::imread(l_path.generic_string());
            if (l_image.empty()) {
              SPDLOG_ERROR("无法读取图片文件: {}", l_path.generic_string());
              continue;
            }
            cv::resize(l_image, l_image, l_size, 0, 0);
            if (auto l_p = l_target_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
            cv::imwrite(l_target_path.generic_string(), l_image);
            SPDLOG_LOGGER_WARN(
                g_logger_ctrl().get_long_task(), "已修复预览文件缩略图: {}", l_target_path.generic_string()
            );
          }
        }
      } catch (...) {
        SPDLOG_LOGGER_ERROR(
            g_logger_ctrl().get_long_task(), "修复预览文件缩略图时发生错误, preview_file_id {} {}", l_preview.uuid_id_,
            boost::current_exception_diagnostic_information()
        );
        continue;
      }
    }
    SPDLOG_LOGGER_INFO(g_logger_ctrl().get_long_task(), "修正完成");
  }
};

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_fix_preview_files_thumbnails, post) {
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始投递修复预览缩略图任务", person_.person_.email_,
      person_.person_.get_full_name()
  );
  boost::asio::post(g_pool_strand(), data_fix_preview_files_thumbnails_run_t{});

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 已投递修复预览缩略图任务", person_.person_.email_,
      person_.person_.get_full_name()
  );
  co_return in_handle->make_msg_204();
}

}  // namespace doodle::http
