#include "doodle_core/core/file_sys.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/attachment_file.h"
#include "doodle_core/metadata/image_size.h"
#include "doodle_core/metadata/project.h"
#include "doodle_core/metadata/task.h"
#include "doodle_core/metadata/task_type.h"
#include "doodle_core/metadata/playlist.h"
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/ffmpeg_video.h>
#include <doodle_lib/core/generate_text_video.hpp>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/comment.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/http_method/kitsu/preview.h>
#include <doodle_lib/http_method/seed_email.h>
#include <doodle_lib/long_task/connect_video.h>

#include <boost/asio/post.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/videoio.hpp>
#include <sqlite_orm/sqlite_orm.h>
#include <vector>

namespace doodle::http {

namespace {
// 先合成视频, 再生成预览

auto compose_video_impl(
    const FSys::path& in_path, const std::size_t& in_fps, const cv::Size& in_size,
    const std::shared_ptr<preview_file>& in_preview_file, const preview_file& in_target_preview_file
) {
  auto l_now             = std::chrono::steady_clock::now();
  auto& l_ctx            = g_ctx().get<kitsu_ctx_t>();
  auto l_target_path     = l_ctx.get_movie_source_file(in_target_preview_file.uuid_id_);

  auto l_new_path        = l_ctx.get_movie_source_file(in_preview_file->uuid_id_);
  auto l_new_backup_path = FSys::add_time_stamp(l_new_path);
  if (auto l_p = l_new_path.parent_path(); !exists(l_p)) FSys::create_directories(l_p);
  {
    cv::VideoCapture l_cap(in_path.generic_string(), cv::CAP_FFMPEG);
    cv::VideoCapture l_target_cap(l_target_path.generic_string(), cv::CAP_FFMPEG);
    DOODLE_CHICK(l_cap.isOpened() && l_target_cap.isOpened(), "无法打开视频文件 {} {}", in_path, l_target_path);

    DOODLE_CHICK(
        l_cap.get(cv::CAP_PROP_FRAME_COUNT) == l_target_cap.get(cv::CAP_PROP_FRAME_COUNT), "视频帧数不匹配 {} {} ",
        in_path, l_target_path
    );

    cv::VideoWriter l_video{
        l_new_backup_path.generic_string(), cv::VideoWriter::fourcc('a', 'v', 'c', '1'),
        boost::numeric_cast<std::double_t>(in_fps), in_size
    };
    l_video.set(cv::VIDEOWRITER_PROP_QUALITY, 90);

    DOODLE_CHICK(l_video.isOpened(), "无法创建视频文件: {} ", l_new_path.generic_string());
    cv::Mat l_frame{};
    cv::Mat l_target_frame{};
    /// 255 color
    cv::Mat l_255_mat{in_size, CV_8UC3, cv::Scalar(255, 255, 255)};
    while (l_cap.read(l_frame) && l_target_cap.read(l_target_frame)) {
      DOODLE_CHICK(!l_frame.empty() && !l_target_frame.empty(), "无法读取视频文件: {} {}", in_path, l_target_path);

      if (l_frame.cols != in_size.width || l_frame.rows != in_size.height) {
        cv::resize(l_frame, l_frame, in_size);
      }
      /// 将 l_frame 通过 滤色叠加模型 叠加到 l_target_frame 上
      /// 结果色 = 255 - [(255 - 基色) × (255 - 混合色)] / 255
      cv::Mat l_result_frame = cv::Mat::zeros(in_size, l_frame.type());
      cv::subtract(l_255_mat, l_frame, l_frame);
      cv::subtract(l_255_mat, l_target_frame, l_target_frame);

      cv::multiply(l_frame, l_target_frame, l_result_frame, 1.0 / 255);
      cv::subtract(l_255_mat, l_result_frame, l_result_frame);

      l_video << l_result_frame;
    }
  }
  FSys::rename(l_new_backup_path, l_new_path);
  SPDLOG_INFO("合成视频完成 {} ", l_new_path);
  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_long_task(), "合成视频完成 {} {:%H:%M:%S}", l_new_path,
      std::chrono::steady_clock::now() - l_now
  );

  preview::handle_video_file(l_new_path, in_fps, in_size, in_preview_file);
}

}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_preview_files_compose_video, post) {
  auto l_file = in_handle->get_file();
  DOODLE_CHICK_HTTP(!l_file.empty() && FSys::exists(l_file), bad_request, "必须上传视频文件");
  auto l_sql          = g_ctx().get<sqlite_database>();
  auto l_preview_file = l_sql.get_by_uuid<preview_file>(preview_file_id_);
  auto l_task         = l_sql.get_by_uuid<task>(l_preview_file.task_id_);
  auto l_project      = l_sql.get_by_uuid<project>(l_task.project_id_);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(),
      "用户 {}({}) 开始合成视频 preview_file_id {} task_id {} project_id {} filename {} ext {}",
      person_.person_.email_, person_.person_.get_full_name(), preview_file_id_, l_preview_file.task_id_,
      l_task.project_id_, l_file.filename().generic_string(), l_file.extension().generic_string()
  );

  preview_file l_target_preview_file{};
  {
    using namespace sqlite_orm;
    auto l_preview_files = l_sql.impl_->storage_any_.get_all<preview_file>(
        join<task>(on(c(&preview_file::task_id_) == c(&task::uuid_id_))),
        where(
            c(&task::entity_id_) == l_task.entity_id_ &&
            in(&task::task_type_id_,
               {task_type::get_simulation_task_id(), task_type::get_lighting_id(), task_type::get_animation_id()}) &&
            c(&preview_file::source_) == preview_file_source_enum::auto_light_generate
        ),
        order_by(&preview_file::created_at_).desc(), limit(1)
    );
    DOODLE_CHICK_HTTP(!l_preview_files.empty(), bad_request, "没有找到相关的预览文件");
    // 选择最新的预览文件
    l_target_preview_file = l_preview_files.front();
  }

  // 检查文件的存在性, 以及长度
  {
    auto l_target_path = g_ctx().get<kitsu_ctx_t>().get_movie_source_file(l_target_preview_file.uuid_id_);
    cv::VideoCapture l_cap(l_file.generic_string(), cv::CAP_FFMPEG);
    cv::VideoCapture l_target_cap(l_target_path.generic_string(), cv::CAP_FFMPEG);
    DOODLE_CHICK_HTTP(
        l_cap.isOpened() && l_target_cap.isOpened(), bad_request, "无法打开视频文件 {} {}", l_file, l_target_path
    );
    auto l_cap_frame_count    = l_cap.get(cv::CAP_PROP_FRAME_COUNT);
    auto l_target_frame_count = l_target_cap.get(cv::CAP_PROP_FRAME_COUNT);
    DOODLE_CHICK_HTTP(
        l_cap_frame_count == l_target_frame_count, bad_request, "视频帧数不匹配 {} != {}", l_cap_frame_count,
        l_target_frame_count
    );
  }

  // 开始合成视频
  auto l_prj_size               = l_project.get_resolution();
  l_preview_file.extension_     = "mp4";
  l_preview_file.original_name_ = l_file.stem().generic_string();
  l_preview_file.width_         = l_prj_size.first;
  l_preview_file.height_        = l_prj_size.second;
  auto l_preview_file_ptr       = std::make_shared<preview_file>(l_preview_file);
  co_await l_sql.update(l_preview_file_ptr);

  boost::asio::post(
      g_strand(), [fps = l_project.fps_, l_prj_size, l_preview_file_ptr, l_target_preview_file, l_file]() mutable {
        compose_video_impl(
            l_file, fps, cv::Size{l_prj_size.first, l_prj_size.second}, l_preview_file_ptr, l_target_preview_file
        );
      }
  );

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(),
      "用户 {}({}) 已投递合成视频任务 preview_file_id {} target_preview_file_id {} width {} height {}",
      person_.person_.email_, person_.person_.get_full_name(), preview_file_id_, l_target_preview_file.uuid_id_,
      l_preview_file_ptr->width_, l_preview_file_ptr->height_
  );

  co_return in_handle->make_msg(nlohmann::json{} = *l_preview_file_ptr);
}

struct actions_playlists_preview_files_create_review_arg {
  // 添加字幕, 混音
  bool add_subtitle_  = false;
  bool add_dubbing_   = false;
  // 添加名称
  bool add_name_      = false;
  // 添加片头片尾
  bool add_head_tail_ = false;
  // 添加水印
  bool add_watermark_ = false;
  // 添加时间码
  bool add_time_code_ = false;

  friend void from_json(const nlohmann::json& in_json, actions_playlists_preview_files_create_review_arg& out_arg) {
    if (in_json.contains("add_subtitle")) in_json.at("add_subtitle").get_to(out_arg.add_subtitle_);
    if (in_json.contains("add_dubbing")) in_json.at("add_dubbing").get_to(out_arg.add_dubbing_);
    if (in_json.contains("add_name")) in_json.at("add_name").get_to(out_arg.add_name_);
    if (in_json.contains("add_head_tail")) in_json.at("add_head_tail").get_to(out_arg.add_head_tail_);
    if (in_json.contains("add_watermark")) in_json.at("add_watermark").get_to(out_arg.add_watermark_);
    if (in_json.contains("add_time_code")) in_json.at("add_time_code").get_to(out_arg.add_time_code_);
  }
};

namespace {

struct run_actions_playlists_preview_files_create_review {
  struct data {
    ffmpeg_video ffmpeg_video_;

    std::vector<FSys::path> shot_preview_paths_{};
    image_size size_{};
    logger_ptr logger_{};

    std::shared_ptr<preview_file> review_preview_file_{};
  };
  std::shared_ptr<data> data_ptr_;

  run_actions_playlists_preview_files_create_review() : data_ptr_(std::make_shared<data>()) {}

  void operator()() {
    data_ptr_->logger_ = g_logger_ctrl().get_long_task();
    if (data_ptr_->shot_preview_paths_.empty()) {
      SPDLOG_LOGGER_WARN(data_ptr_->logger_, "没有找到任何镜头预览视频, 无法生成评审视频");
      return;
    }
    // 先连接视频
    auto l_tmp = core_set::get_set().get_cache_root("compose_review_tmp") /
                 fmt::format("{}.mp4", core_set::get_set().get_uuid());
    auto l_now = std::chrono::steady_clock::now();
    doodle::detail::connect_video(l_tmp, data_ptr_->logger_, data_ptr_->shot_preview_paths_, data_ptr_->size_);
    SPDLOG_LOGGER_WARN(
        g_logger_ctrl().get_long_task(), "连接视频完成 {} {:%H:%M:%S}", l_tmp, std::chrono::steady_clock::now() - l_now
    );
    // 再处理视频
    auto l_out_path = g_ctx().get<kitsu_ctx_t>().get_movie_source_file(data_ptr_->review_preview_file_->uuid_id_);
    auto l_out_backup_path = FSys::add_time_stamp(l_out_path);
    if (auto l_p = l_out_path.parent_path(); !exists(l_p)) FSys::create_directories(l_p);
    data_ptr_->ffmpeg_video_.set_input_video(l_tmp);
    data_ptr_->ffmpeg_video_.set_output_video(l_out_backup_path);
    l_now = std::chrono::steady_clock::now();
    data_ptr_->ffmpeg_video_.process();
    SPDLOG_LOGGER_INFO(
        data_ptr_->logger_, "生成评审视频完成 {} {:%H:%M:%S}", l_out_path, std::chrono::steady_clock::now() - l_now
    );
    // 替换文件
    FSys::rename(l_out_backup_path, l_out_path);
    // 删除临时文件
    FSys::remove(l_tmp);
    auto l_sql = g_ctx().get<sqlite_database>();
    // 更新预览文件信息
    cv::Size size_{data_ptr_->size_.width, data_ptr_->size_.height};
    auto l_prj =
        l_sql.get_by_uuid<project>(l_sql.get_by_uuid<task>(data_ptr_->review_preview_file_->task_id_).project_id_);
    preview::handle_video_file(l_out_path, l_prj.fps_, size_, data_ptr_->review_preview_file_);

    SPDLOG_LOGGER_INFO(data_ptr_->logger_, "生成评审视频完成 {}", l_out_path);
  }
};

}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_playlists_preview_files_create_review, post) {
  auto l_sql           = g_ctx().get<sqlite_database>();
  auto l_playlist      = l_sql.get_by_uuid<playlist>(playlist_id_);
  auto l_playlist_shot = l_sql.get_playlist_shot_entity(playlist_id_);
  auto l_preview_file  = l_sql.get_by_uuid<preview_file>(preview_file_id_);
  person_.check_in_project(l_playlist.project_id_);
  person_.check_not_outsourcer();
  DOODLE_CHICK_HTTP(!l_playlist_shot.empty(), bad_request, "播放列表中没有任何镜头, 无法生成评审视频");

  auto l_task = l_sql.get_by_uuid<task>(l_preview_file.task_id_);
  auto l_arg  = in_handle->get_json().get<actions_playlists_preview_files_create_review_arg>();
    SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(),
      "用户 {}({}) 开始生成播放列表评审视频 playlist_id {} preview_file_id {} project_id {} shot_count {} subtitle {} dubbing {} name {} head_tail {} watermark {} time_code {}",
      person_.person_.email_, person_.person_.get_full_name(), playlist_id_, preview_file_id_, l_task.project_id_,
      l_playlist_shot.size(), l_arg.add_subtitle_, l_arg.add_dubbing_, l_arg.add_name_, l_arg.add_head_tail_,
      l_arg.add_watermark_, l_arg.add_time_code_
    );
  using namespace sqlite_orm;
  auto l_attachment_files = l_sql.impl_->storage_any_.get_all<attachment_file>(where(
      in(&attachment_file::comment_id_,
         select(&comment::uuid_id_, from<comment>(), where(c(&comment::object_id_) == l_task.uuid_id_)))
  ));
  // 反转 l_attachment_files
  std::reverse(l_attachment_files.begin(), l_attachment_files.end());
  auto l_prj              = l_sql.get_by_uuid<project>(l_task.project_id_);
  run_actions_playlists_preview_files_create_review l_run{};
  l_run.data_ptr_->size_                = l_prj.get_resolution();
  l_run.data_ptr_->logger_              = in_handle->logger_;
  l_run.data_ptr_->review_preview_file_ = std::make_shared<preview_file>(l_preview_file);
  // 配置 ffmpeg_video
  if (l_arg.add_subtitle_) {
    auto l_subtitle_file =
        std::find_if(l_attachment_files.begin(), l_attachment_files.end(), [](const attachment_file& in_file) {
          return in_file.name_.ends_with(".srt");
        });
    DOODLE_CHICK_HTTP(l_subtitle_file != l_attachment_files.end(), bad_request, "没有找到字幕文件");
    l_run.data_ptr_->ffmpeg_video_.set_subtitle(
        g_ctx().get<kitsu_ctx_t>().get_attachment_file(l_subtitle_file->uuid_id_)
    );
  }
  if (l_arg.add_dubbing_) {
    auto l_dubbing_file =
        std::find_if(l_attachment_files.begin(), l_attachment_files.end(), [](const attachment_file& in_file) {
          return in_file.name_.ends_with(".wav");
        });
    DOODLE_CHICK_HTTP(l_dubbing_file != l_attachment_files.end(), bad_request, "没有找到配音文件");
    l_run.data_ptr_->ffmpeg_video_.set_audio(g_ctx().get<kitsu_ctx_t>().get_attachment_file(l_dubbing_file->uuid_id_));
  }
  if (l_arg.add_name_) {
    auto l_name_file =
        std::find_if(l_attachment_files.begin(), l_attachment_files.end(), [](const attachment_file& in_file) {
          return in_file.name_.ends_with("_episode.mp4");
        });
    DOODLE_CHICK_HTTP(l_name_file != l_attachment_files.end(), bad_request, "没有找到名称视频文件");
    l_run.data_ptr_->ffmpeg_video_.set_episodes_name(
        g_ctx().get<kitsu_ctx_t>().get_attachment_file(l_name_file->uuid_id_)
    );
  }
  if (l_arg.add_head_tail_) {
    auto l_head_file =
        std::find_if(l_attachment_files.begin(), l_attachment_files.end(), [](const attachment_file& in_file) {
          return in_file.name_.ends_with("_intro.mp4");
        });
    DOODLE_CHICK_HTTP(l_head_file != l_attachment_files.end(), bad_request, "没有找到片头视频文件");
    l_run.data_ptr_->ffmpeg_video_.set_intro(g_ctx().get<kitsu_ctx_t>().get_attachment_file(l_head_file->uuid_id_));
    auto l_tail_file =
        std::find_if(l_attachment_files.begin(), l_attachment_files.end(), [](const attachment_file& in_file) {
          return in_file.name_.ends_with("_outro.mp4");
        });
    DOODLE_CHICK_HTTP(l_tail_file != l_attachment_files.end(), bad_request, "没有找到片尾视频文件");
    l_run.data_ptr_->ffmpeg_video_.set_outro(g_ctx().get<kitsu_ctx_t>().get_attachment_file(l_tail_file->uuid_id_));
  }
  if (l_arg.add_watermark_) {
    l_run.data_ptr_->ffmpeg_video_.set_watermark("送审样片");
  }
  if (l_arg.add_time_code_) {
    l_run.data_ptr_->ffmpeg_video_.set_time_code(true);
  }

  std::vector<FSys::path> l_paths{};
  for (const auto& l_shot_preview : l_playlist_shot) {
    auto l_path = g_ctx().get<kitsu_ctx_t>().get_movie_source_file(l_shot_preview.preview_id_);
    if (FSys::exists(l_path)) l_paths.push_back(l_path);
  }
  DOODLE_CHICK_HTTP(!l_paths.empty(), bad_request, "没有找到任何镜头预览视频, 无法生成评审视频");
  l_run.data_ptr_->shot_preview_paths_ = l_paths;
  boost::asio::post(g_strand(), l_run);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(),
      "用户 {}({}) 已投递生成播放列表评审视频任务 playlist_id {} preview_file_id {} shot_count {} attachment_count {} video_input_count {}",
      person_.person_.email_, person_.person_.get_full_name(), playlist_id_, preview_file_id_, l_playlist_shot.size(),
      l_attachment_files.size(), l_paths.size()
  );

  co_return in_handle->make_msg(nlohmann::json{} = l_playlist_shot);
}
struct actions_tasks_create_review_args {
  FSys::path subtitle_path_;
  FSys::path audio_path_;
  FSys::path intro_path_;
  FSys::path outro_path_;
  // 集数名称
  std::string episodes_name_;
  FSys::path episodes_name_path_;

  friend void from_json(const nlohmann::json& in_json, actions_tasks_create_review_args& out_arg) {
    if (in_json.contains("subtitle_path") && in_json.at("subtitle_path").is_string())
      in_json.at("subtitle_path").get_to(out_arg.subtitle_path_);
    if (in_json.contains("audio_path") && in_json.at("audio_path").is_string())
      in_json.at("audio_path").get_to(out_arg.audio_path_);
    if (in_json.contains("intro_path") && in_json.at("intro_path").is_string())
      in_json.at("intro_path").get_to(out_arg.intro_path_);
    if (in_json.contains("outro_path") && in_json.at("outro_path").is_string())
      in_json.at("outro_path").get_to(out_arg.outro_path_);
    if (in_json.contains("episodes_name") && in_json.at("episodes_name").is_string())
      in_json.at("episodes_name").get_to(out_arg.episodes_name_);
  }
};

struct actions_tasks_create_review_run {
  struct data {
    logger_ptr logger_{};
    actions_tasks_create_review_args args_{};
  };
  std::shared_ptr<data> data_ptr_;

  actions_tasks_create_review_run() : data_ptr_(std::make_shared<data>()) {}

  void operator()() {
    if (!data_ptr_->args_.episodes_name_path_.empty()) {
      if (auto l_p = data_ptr_->args_.episodes_name_path_.parent_path(); !FSys::exists(l_p))
        FSys::create_directories(l_p);
      if (FSys::exists(data_ptr_->args_.episodes_name_path_)) FSys::remove(data_ptr_->args_.episodes_name_path_);
      // todo: 修正集数名称
      auto l_now = std::chrono::steady_clock::now();
      data_ptr_->logger_->info("开始修正集数名称");
      generate_text_video l_generator{};

      l_generator.set_out_path(data_ptr_->args_.episodes_name_path_);
      auto l_it =
          std::find_if(data_ptr_->args_.episodes_name_.begin(), data_ptr_->args_.episodes_name_.end(), [](char in_c) {
            return in_c == '\n';
          });
      std::string l_title =
          data_ptr_->args_.episodes_name_.substr(0, std::distance(data_ptr_->args_.episodes_name_.begin(), l_it));
      std::string l_subtitle =
          data_ptr_->args_.episodes_name_.substr(std::distance(data_ptr_->args_.episodes_name_.begin(), l_it) + 1);
      l_generator.add_font_attr(
          generate_text_video::font_attr_t{
              .font_path_   = "D:/No.21-上首传奇书法体.ttf",
              .font_height_ = 80,
              .font_point_  = cv::Point{0, 395},
              .text_        = l_title
          }
      );
      l_generator.add_font_attr(
          generate_text_video::font_attr_t{
              .font_path_   = "D:/No.21-上首传奇书法体.ttf",
              .font_height_ = 110,
              .font_point_  = cv::Point{0, 395 + 80 + 100},
              .text_        = l_subtitle
          }
      );
      l_generator.run();

      data_ptr_->logger_->info(
          "集数名称生成完成 {} {:%H:%M:%S}", data_ptr_->args_.episodes_name_path_,
          std::chrono::steady_clock::now() - l_now
      );
      SPDLOG_LOGGER_WARN(
          g_logger_ctrl().get_long_task(), "集数名称生成完成 {} {:%H:%M:%S}", data_ptr_->args_.episodes_name_path_,
          std::chrono::steady_clock::now() - l_now
      );
    }
  }
};

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_tasks_create_review, post) {
  std::shared_ptr<comment> l_comment = std::make_shared<comment>();
  auto l_json                        = in_handle->get_json();
  actions_tasks_create_review_run l_run;
  l_json.get_to(l_run.data_ptr_->args_);
  l_json.get_to(*l_comment);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(),
      "用户 {}({}) 开始创建评审任务评论 task_id {} subtitle {} audio {} intro {} outro {} episodes_name {}",
      person_.person_.email_, person_.person_.get_full_name(), task_id_, !l_run.data_ptr_->args_.subtitle_path_.empty(),
      !l_run.data_ptr_->args_.audio_path_.empty(), !l_run.data_ptr_->args_.intro_path_.empty(),
      !l_run.data_ptr_->args_.outro_path_.empty(), !l_run.data_ptr_->args_.episodes_name_.empty()
  );

  if (!l_run.data_ptr_->args_.subtitle_path_.empty() && FSys::exists(l_run.data_ptr_->args_.subtitle_path_) &&
      l_run.data_ptr_->args_.subtitle_path_.extension() != ".srt") {
    auto l_new_path = l_run.data_ptr_->args_.subtitle_path_;
    l_new_path.replace_extension(".srt");
    FSys::rename(l_run.data_ptr_->args_.subtitle_path_, l_new_path);
    l_run.data_ptr_->args_.subtitle_path_ = l_new_path;
  }
  if (!l_run.data_ptr_->args_.audio_path_.empty() && FSys::exists(l_run.data_ptr_->args_.audio_path_)) {
    ffmpeg_video::check_video_valid(l_run.data_ptr_->args_.audio_path_, "配音轨道", false);
    if (l_run.data_ptr_->args_.audio_path_.extension() != ".wav") {
      auto l_new_path = l_run.data_ptr_->args_.audio_path_;
      l_new_path.replace_extension(".wav");
      FSys::rename(l_run.data_ptr_->args_.audio_path_, l_new_path);
      l_run.data_ptr_->args_.audio_path_ = l_new_path;
    }
  }
  if (!l_run.data_ptr_->args_.intro_path_.empty() && FSys::exists(l_run.data_ptr_->args_.intro_path_)) {
    auto l_new_path = l_run.data_ptr_->args_.intro_path_.parent_path() /
                      fmt::format("{}_intro.mp4", l_run.data_ptr_->args_.intro_path_.stem().generic_string());
    l_new_path.replace_extension(".mp4");
    FSys::rename(l_run.data_ptr_->args_.intro_path_, l_new_path);
    l_run.data_ptr_->args_.intro_path_ = l_new_path;
    ffmpeg_video::check_video_valid(l_run.data_ptr_->args_.intro_path_, "片头视频");
  }
  if (!l_run.data_ptr_->args_.outro_path_.empty() && FSys::exists(l_run.data_ptr_->args_.outro_path_)) {
    auto l_new_path = l_run.data_ptr_->args_.outro_path_.parent_path() /
                      fmt::format("{}_outro.mp4", l_run.data_ptr_->args_.outro_path_.stem().generic_string());
    l_new_path.replace_extension(".mp4");
    FSys::rename(l_run.data_ptr_->args_.outro_path_, l_new_path);
    l_run.data_ptr_->args_.outro_path_ = l_new_path;
    ffmpeg_video::check_video_valid(l_run.data_ptr_->args_.outro_path_, "片尾视频");
  }
  if (!l_run.data_ptr_->args_.episodes_name_.empty()) {
    // 检查名称合法性, 必须是带 \n换行符的两行文本
    auto l_it = std::find_if(
        l_run.data_ptr_->args_.episodes_name_.begin(), l_run.data_ptr_->args_.episodes_name_.end(),
        [](char in_c) { return in_c == '\n'; }
    );
    DOODLE_CHICK_HTTP(l_it != l_run.data_ptr_->args_.episodes_name_.end(), bad_request, "集数名称必须包含换行符");
  }

  std::vector<FSys::path> l_files{};
  // 生成集数临时文件(真正生成会推后到)
  if (!l_run.data_ptr_->args_.episodes_name_.empty()) {
    auto l_tmp_path = core_set::get_set().get_cache_root("episode_name_tmp") /
                      fmt::format("{}_episode.mp4", core_set::get_set().get_uuid());
    FSys::create_directories(l_tmp_path.parent_path());
    FSys::ofstream{l_tmp_path} << l_run.data_ptr_->args_.episodes_name_;
    l_files.push_back(l_tmp_path);
    l_run.data_ptr_->args_.episodes_name_path_ = l_tmp_path;
  }
  if (l_comment->text_.empty()) l_comment->text_ = "创建评审附件";

  if (!l_run.data_ptr_->args_.subtitle_path_.empty() && FSys::exists(l_run.data_ptr_->args_.subtitle_path_))
    l_files.push_back(l_run.data_ptr_->args_.subtitle_path_);
  if (!l_run.data_ptr_->args_.audio_path_.empty() && FSys::exists(l_run.data_ptr_->args_.audio_path_))
    l_files.push_back(l_run.data_ptr_->args_.audio_path_);
  if (!l_run.data_ptr_->args_.intro_path_.empty() && FSys::exists(l_run.data_ptr_->args_.intro_path_))
    l_files.push_back(l_run.data_ptr_->args_.intro_path_);
  if (!l_run.data_ptr_->args_.outro_path_.empty() && FSys::exists(l_run.data_ptr_->args_.outro_path_))
    l_files.push_back(l_run.data_ptr_->args_.outro_path_);

  auto l_result   = co_await create_comment(l_comment, &person_, task_id_, l_files);

  // 重新定向文件
  auto l_fix_path = [&](const FSys::path& in_path) {
    if (in_path.empty()) return FSys::path{};
    auto l_it = std::find_if(
        l_result.attachment_file_.begin(), l_result.attachment_file_.end(),
        [&](const attachment_file& in_attachment_file) {
          return in_path.filename().generic_string() == in_attachment_file.name_;
        }
    );
    DOODLE_CHICK(l_it != l_result.attachment_file_.end(), "无法找到附件文件 {}", in_path);

    return g_ctx().get<kitsu_ctx_t>().get_attachment_file(l_it->uuid_id_);
  };
  l_run.data_ptr_->logger_                   = in_handle->logger_;
  l_run.data_ptr_->args_.audio_path_         = l_fix_path(l_run.data_ptr_->args_.audio_path_);
  l_run.data_ptr_->args_.subtitle_path_      = l_fix_path(l_run.data_ptr_->args_.subtitle_path_);
  l_run.data_ptr_->args_.intro_path_         = l_fix_path(l_run.data_ptr_->args_.intro_path_);
  l_run.data_ptr_->args_.outro_path_         = l_fix_path(l_run.data_ptr_->args_.outro_path_);
  l_run.data_ptr_->args_.episodes_name_path_ = l_fix_path(l_run.data_ptr_->args_.episodes_name_path_);
  // 删除临时文件
  if (!l_run.data_ptr_->args_.episodes_name_path_.empty() && FSys::exists(l_run.data_ptr_->args_.episodes_name_path_))
    FSys::remove(l_run.data_ptr_->args_.episodes_name_path_);

  boost::asio::post(g_strand(), l_run);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(),
      "用户 {}({}) 完成创建评审任务评论 task_id {} comment_id {} attachment_count {}",
      person_.person_.email_, person_.person_.get_full_name(), task_id_, l_comment->uuid_id_,
      l_result.attachment_file_.size()
  );
  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}

}  // namespace doodle::http