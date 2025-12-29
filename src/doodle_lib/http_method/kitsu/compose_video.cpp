#include "doodle_core/core/file_sys.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/task.h"
#include "doodle_core/metadata/task_type.h"
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/ffmpeg_video.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/http_method/kitsu/preview.h>
#include <doodle_lib/http_method/seed_email.h>

#include <memory>
#include <opencv2/core/mat.hpp>
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

  co_return in_handle->make_msg(nlohmann::json{} = *l_preview_file_ptr);
}

struct actions_preview_files_create_review_arg {
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

  friend void from_json(const nlohmann::json& in_json, actions_preview_files_create_review_arg& out_arg) {
    if (in_json.contains("add_subtitle")) in_json.at("add_subtitle").get_to(out_arg.add_subtitle_);
    if (in_json.contains("add_dubbing")) in_json.at("add_dubbing").get_to(out_arg.add_dubbing_);
    if (in_json.contains("add_name")) in_json.at("add_name").get_to(out_arg.add_name_);
    if (in_json.contains("add_head_tail")) in_json.at("add_head_tail").get_to(out_arg.add_head_tail_);
    if (in_json.contains("add_watermark")) in_json.at("add_watermark").get_to(out_arg.add_watermark_);
    if (in_json.contains("add_time_code")) in_json.at("add_time_code").get_to(out_arg.add_time_code_);
  }
};

namespace {
auto get_sequence_shots_preview(const uuid& in_sequence_id) {
  auto l_sql = g_ctx().get<sqlite_database>();
  using namespace sqlite_orm;

  constexpr auto sequence = "sequence"_alias.for_<entity>();
  constexpr auto episode  = "episode"_alias.for_<entity>();
  auto l_shots            = l_sql.impl_->storage_any_.select(
      columns(object<preview_file>(true), &entity::uuid_id_), from<preview_file>(),
      join<task>(on(c(&preview_file::task_id_) == c(&task::uuid_id_))),
      join<entity>(on(c(&task::entity_id_) == c(&entity::uuid_id_))),
      join<sequence>(on(c(&entity::parent_id_) == c(sequence->*&entity::uuid_id_))),
      // join<episode>(on(c(&entity::parent_id_) == c(episode->*&entity::uuid_id_))),
      where(
          c(sequence->*&entity::uuid_id_) == in_sequence_id &&
          (c(&preview_file::source_) == preview_file_source_enum::auto_light_generate ||
           c(&preview_file::source_) == preview_file_source_enum::vfx_review)
      ),
      order_by(&preview_file::created_at_).desc()
  );
}
}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_preview_files_create_review, post) {
  auto l_sql          = g_ctx().get<sqlite_database>();
  auto l_preview_file = l_sql.get_by_uuid<preview_file>(preview_file_id_);
  auto l_task         = l_sql.get_by_uuid<task>(l_preview_file.task_id_);
  auto l_arg          = in_handle->get_json().get<actions_preview_files_create_review_arg>();
  using namespace sqlite_orm;
  auto l_attachment_files = l_sql.impl_->storage_any_.get_all<attachment_file>(where(
      c(&attachment_file::comment_id_) ==
      select(&comment::uuid_id_, from<comment>(), where(c(&comment::object_id_) == l_task.uuid_id_))
  ));

  auto l_ffmpeg_video     = std::make_shared<ffmpeg_video>();

  if (l_arg.add_subtitle_) {
    auto l_subtitle_file =
        std::find_if(l_attachment_files.begin(), l_attachment_files.end(), [](const attachment_file& in_file) {
          return in_file.name_.ends_with(".srt");
        });
    DOODLE_CHICK_HTTP(l_subtitle_file != l_attachment_files.end(), bad_request, "没有找到字幕文件");
    l_ffmpeg_video->set_subtitle(g_ctx().get<kitsu_ctx_t>().get_attachment_file(l_subtitle_file->uuid_id_));
  }
  if (l_arg.add_dubbing_) {
    auto l_dubbing_file =
        std::find_if(l_attachment_files.begin(), l_attachment_files.end(), [](const attachment_file& in_file) {
          return in_file.name_.starts_with("audio");
        });
    DOODLE_CHICK_HTTP(l_dubbing_file != l_attachment_files.end(), bad_request, "没有找到配音文件");
    l_ffmpeg_video->set_audio(g_ctx().get<kitsu_ctx_t>().get_attachment_file(l_dubbing_file->uuid_id_));
  }

  co_return in_handle->make_msg(nlohmann::json{} = l_attachment_files);
}

}  // namespace doodle::http