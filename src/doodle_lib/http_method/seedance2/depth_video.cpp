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

#include <doodle_lib/ai/depth_anything/doodle_depth_estimation.h>
#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu.h>
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

// PIMPL — 深度估计模型只加载一次，通过 clone() 的 shared_ptr 共享
class doodle_ai_depth_estimation_video::impl {
 public:
  ai::doodle_depth_estimation estimator_;
  FSys::path model_path_;
  explicit impl(const std::filesystem::path& in_path) : estimator_(), model_path_(in_path) {}

  // 后台异步深度估计 — 遵循 task.cpp:289-292 的 run_sql + broadcast 模式, 使用 g_strand() 保证线程安全
  boost::asio::awaitable<void> run_depth_estimation(
      std::shared_ptr<sd2::ai_entity_reference_preview> in_ref, std::shared_ptr<sd2::ai_preview_file> in_preview,
      uuid in_entity_id, FSys::path in_input_path, FSys::path in_output_path, FSys::path in_thumbnail_path
  ) {
    // 在需要时加载
    if (!estimator_) estimator_ = std::move(ai::doodle_depth_estimation{model_path_, false});
    // 1. 打开临时视频，逐帧推理，直接写入最终路径
    auto l_capture = cv::VideoCapture{in_input_path.generic_string()};
    auto l_fps     = l_capture.get(cv::CAP_PROP_FPS);
    auto l_width   = static_cast<int>(l_capture.get(cv::CAP_PROP_FRAME_WIDTH));
    auto l_height  = static_cast<int>(l_capture.get(cv::CAP_PROP_FRAME_HEIGHT));

    auto l_writer  = cv::VideoWriter{
        in_output_path.generic_string(), cv::VideoWriter::fourcc('m', 'p', '4', 'v'), l_fps, cv::Size{l_width, l_height}
    };

    cv::Mat l_frame, l_depth, l_depth_color;
    bool l_first_frame = true;
    while (l_capture.read(l_frame)) {
      l_depth = estimator_.predict(l_frame);
      cv::normalize(l_depth, l_depth, 0, 255, cv::NORM_MINMAX, CV_8U);
      cv::applyColorMap(l_depth, l_depth_color, cv::COLORMAP_TURBO);
      l_writer.write(l_depth_color);

      // 2. 第一帧同时生成缩略图
      if (l_first_frame) {
        l_first_frame = false;
        if (auto l_p = in_thumbnail_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
        auto l_resize = std::min(500.0 / l_depth_color.cols, 500.0 / l_depth_color.rows);
        cv::Mat l_thumb;
        cv::resize(l_depth_color, l_thumb, cv::Size(l_depth_color.cols * l_resize, l_depth_color.rows * l_resize));
        cv::imwrite(in_thumbnail_path.generic_string(), l_thumb);
      }
    }

    // 3. 释放 VideoCapture 后才能删除临时文件
    l_capture.release();
    l_writer.release();
    FSys::remove(in_input_path);

    // 4. 广播完成 — 使用已有的 reference / preview id
    socket_io::broadcast(
        socket_io::seedance2_entity_reference_new_broadcast_t{
            .reference_id_ = in_ref->uuid_id_, .entity_id_ = in_entity_id, .preview_file_id_ = in_preview->uuid_id_
        }
    );
    co_return;
  }
};

doodle_ai_depth_estimation_video::doodle_ai_depth_estimation_video() : depth_impl_(nullptr) {
  depth_impl_ = std::make_shared<impl>(g_ctx().get<kitsu_ctx_t>().get_depth_model_path());
}

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

  // 2. 一次提交两个 insert — 遵循 ai_episode.cpp 模式
  co_await l_sql.run_sql(l_install_1, l_install_2);

  // 3. 存储路径 — 原始视频暂存临时文件，转换后输出到最终路径
  auto& l_ctx           = g_ctx().get<kitsu_ctx_t>();
  auto l_file_picture   = l_ctx.get_sd2_pictures_file(l_preview->uuid_id_, l_ext);
  auto l_file_thumbnail = l_ctx.get_sd2_thumbnail_file(l_preview->uuid_id_);
  auto l_file_tmp       = l_file_picture;
  l_file_tmp.replace_extension(".upload_tmp.mp4");

  if (auto l_p = l_file_picture.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);

  // 4. 保存原始视频到临时文件
  FSys::rename(l_file, l_file_tmp);

  // 5. 异步触发深度估计（不阻塞响应）
  boost::asio::co_spawn(
      g_strand(),
      depth_impl_->run_depth_estimation(l_ref, l_preview, entity_id_, l_file_tmp, l_file_picture, l_file_thumbnail),
      boost::asio::consign(boost::asio::detached, http_connection_guard{})
  );

  co_return in_handle->make_msg(nlohmann::json{{"reference", *l_ref}, {"preview", *l_preview}});
}

}  // namespace doodle::http::seedance2