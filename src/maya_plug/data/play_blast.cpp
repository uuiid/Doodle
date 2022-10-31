//
// Created by TD on 2021/11/22.
//

#include "play_blast.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/metadata/move_create.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/thread_pool/image_to_movie.h>

#include <maya_plug/data/maya_camera.h>

#include <fmt/chrono.h>
#include <fmt/ostream.h>
#include <maya/M3dView.h>
#include <maya/MAnimControl.h>
#include <maya/MDrawContext.h>
#include <maya/MFileIO.h>
#include <maya/MGlobal.h>
namespace doodle::maya_plug {

std::string play_blast::p_post_render_notification_name{"doodle_lib_maya_notification_name"};

// void play_blast::captureCallback(MHWRender::MDrawContext& context, void* clientData) {
//   MStatus k_s{};
//   auto self     = static_cast<play_blast*>(clientData);
//   auto k_render = MHWRender::MRenderer::theRenderer();
//   if (!self || !k_render) return;
//
//   auto k_p = self->get_file_path(self->p_current_time);
//
//   MString k_path{};
//   k_path.setUTF8(k_p.generic_string().c_str());
//
//   if (k_path.length() == 0)
//     return;
//   auto k_texManager = k_render->getTextureManager();
//   auto k_tex        = context.copyCurrentColorRenderTargetToTexture();
//   if (k_tex) {
//     k_s = k_texManager->saveTexture(k_tex, k_path);
//     k_texManager->releaseTexture(k_tex);
//   }
//
//   MString k_tmp{};
//   if (!k_s) {
//     k_tmp = k_s.errorString();
//     k_tmp += " error path -> ";
//   } else {
//     k_tmp.setUTF8("捕获的颜色渲染目标到: ");
//   }
// }

play_blast::play_blast()
    : p_save_path(core_set::get_set().get_cache_root("maya_play_blast")),
      p_eps(),
      p_shot(),
      p_current_time(),
      p_uuid() {}
FSys::path play_blast::get_file_dir() const {
  auto k_cache_path = core_set::get_set().get_cache_root("maya_play_blast/tmp");
  k_cache_path /= p_uuid.substr(0, 3);
  if (!FSys::exists(k_cache_path)) {
    FSys::create_directories(k_cache_path);
  }
  return k_cache_path;
}

FSys::path play_blast::get_file_path(const MTime& in_time) const {
  auto k_cache_path = get_file_dir();
  k_cache_path /= fmt::format("{}_{:05d}.png", p_uuid, boost::numeric_cast<std::int32_t>(in_time.as(MTime::uiUnit())));
  return k_cache_path;
}
FSys::path play_blast::get_file_path() const {
  auto k_cache_path = get_file_dir();
  k_cache_path /= fmt::format("{}_", p_uuid);
  return k_cache_path;
}
FSys::path play_blast::set_save_path(const FSys::path& in_save_path) {
  p_save_path = in_save_path;
  return get_out_path();
}
FSys::path play_blast::set_save_dir(const FSys::path& in_save_dir) {
  p_save_path = in_save_dir / p_save_path.filename();
  return get_out_path();
}

FSys::path play_blast::set_save_filename(const FSys::path& in_save_filename) {
  p_save_path.remove_filename() /= in_save_filename;
  return get_out_path();
}

FSys::path play_blast::get_out_path() const {
  if (!FSys::exists(p_save_path.parent_path())) FSys::create_directories(p_save_path.parent_path());
  // k_cache_path /= fmt::format("{}_{}.mp4", p_eps, p_shot);
  return p_save_path;
}

bool play_blast::conjecture_camera() {
  auto& k_cam = g_reg()->ctx().emplace<maya_camera>();
  k_cam.conjecture();
  return true;
}

MStatus play_blast::play_blast_(const MTime& in_start, const MTime& in_end) {
  p_uuid = core_set::get_set().get_uuid_str();
  MStatus k_s{};

  if (!g_reg()->ctx().contains<maya_camera>()) {
    g_reg()->ctx().emplace<maya_camera>().conjecture();
  }

  auto& k_cam = g_reg()->ctx().at<maya_camera>();
  k_cam.set_render_cam();
  k_cam.set_play_attr();

  k_s = MGlobal::executeCommand(R"(colorManagementPrefs -e -outputTransformEnabled true -outputTarget "renderer";
  colorManagementPrefs -e -outputUseViewTransform -outputTarget "renderer";)");
  CHECK_MSTATUS_AND_RETURN_IT(k_s);

  if (MGlobal::mayaState(&k_s) == MGlobal::kInteractive) {
    auto k_view = M3dView::active3dView(&k_s);
    if (k_s) {
      k_s = k_view.setCamera(k_cam.p_path);
      CHECK_MSTATUS(k_s);
      if (!k_s) {
        DOODLE_LOG_WARN("not set cam view");
      }
      k_view.setDisplayStyle(M3dView::DisplayStyle::kGouraudShaded);
      k_view.setObjectDisplay(M3dView::DisplayObjects::kDisplayLocators | M3dView::DisplayObjects::kDisplayMeshes);
      k_view.refresh(false, true);
    } else {
      DOODLE_LOG_WARN("not find view");
    }
    auto k_mel = fmt::format(
        R"(playblast
-compression "H.264"
-filename "{}"
-format "qt"
-height 1080
-percent 100
-quality 100
-viewer false
-forceOverwrite
-width 1920
-startTime {}
-endTime {}
;
)",
        get_out_path().replace_extension("").generic_string(), in_start.as(MTime::uiUnit()), in_end.as(MTime::uiUnit())
    );
    k_s = MGlobal::executeCommand(k_mel.c_str());
    CHECK_MSTATUS_AND_RETURN_IT(k_s);
    return MStatus::kSuccess;
  } else {
    auto k_mel = fmt::format(
        R"(playblast
-compression "png"
-filename "{}"
-format "image"
-height 1080
-offScreen
-percent 100
-quality 100
-viewer false
-width 1920
-startTime {}
-endTime {}
;
)",
        get_file_path().generic_string(), in_start.value(), in_end.value()
    );
    DOODLE_LOG_INFO("开始生成图片序列 {}\n{}", get_file_path(), k_mel);
    k_s = MGlobal::executeCommand(k_mel.c_str(), true, false);
    CHECK_MSTATUS_AND_RETURN_IT(k_s);
    MGlobal::executeCommand(R"(colorManagementPrefs -e -outputTransformEnabled false -outputTarget "renderer";)");

    auto k_f = get_file_dir();

    std::vector<movie::image_attr> l_handle_list{};
    std::double_t k_frame{0};
    for (auto& l_path : FSys::directory_iterator{k_f}) {
      movie::image_attr k_image{};
      k_image.path_attr = l_path;
      /// \brief 相机名称
      k_image.watermarks_attr.emplace_back(
          k_cam.get_transform_name(), 0.1, 0.1, movie::image_watermark::rgba_t{25, 220, 2}
      );
      /// \brief 当前帧和总帧数
      auto k_len = in_end - in_start + 1;
      k_image.watermarks_attr.emplace_back(
          fmt::format("{}/{}", in_start.value() + k_frame, k_len.value()), 0.5, 0.1,
          movie::image_watermark::rgba_t{25, 220, 2}
      );
      ++k_frame;
      /// \brief 绘制摄像机avo
      k_image.watermarks_attr.emplace_back(
          fmt::format("FOV: {:.3f}", k_cam.focalLength()), 0.91, 0.1, movie::image_watermark::rgba_t{25, 220, 2}
      );
      /// \brief 当前时间节点
      k_image.watermarks_attr.emplace_back(
          fmt::format("{:%Y-%m-%d %H:%M:%S}", chrono::floor<chrono::minutes>(chrono::system_clock::now())), 0.1, 0.91,
          movie::image_watermark::rgba_t{25, 220, 2}
      );
      /// \brief 制作人姓名
      k_image.watermarks_attr.emplace_back(
          g_reg()->ctx().at<user::current_user>().user_name_attr(), 0.5, 0.91,
          movie::image_watermark::rgba_t{25, 220, 2}
      );
      l_handle_list.push_back(std::move(k_image));
    }
    auto k_msg = make_handle();
    k_msg.emplace<process_message>().set_name("制作拍屏");
    k_msg.emplace<FSys::path>(get_out_path());
    k_msg.emplace<episodes>(p_eps);
    k_msg.emplace<shot>(p_shot);

    if (MGlobal::mayaState(&k_s) != MGlobal::kInteractive) {
      DOODLE_LOG_INFO("检查为非交互模式, 进行同步视频合成");
      bool l_ok{};
      g_reg()->ctx().at<image_to_move>()->async_create_move(k_msg, l_handle_list, [l_r = &l_ok, this, k_f]() {
        *l_r = true;
        DOODLE_LOG_INFO("完成视频合成 {} , 并删除图片 {}", get_out_path(), k_f);
        FSys::remove_all(k_f);
      });
      while (!l_ok) {
        g_io_context().poll_one();
      }

    } else {
      g_reg()->ctx().at<image_to_move>()->async_create_move(k_msg, l_handle_list, [this, k_f]() {
        DOODLE_LOG_INFO("完成视频合成 {} , 并删除图片 {}", get_out_path(), k_f);
        FSys::remove_all(k_f);
      });
    }
    return k_s;
  }
}

bool play_blast::conjecture_ep_sc() {
  FSys::path p_current_path{MFileIO::currentFile().asUTF8()};
  auto k_r = p_eps.analysis(p_current_path) && p_shot.analysis(p_current_path);
  set_save_filename(p_current_path.filename().replace_extension(".mp4"));
  return k_r;
}
void play_blast::play_blast_by_render(const MTime& in_start, const MTime& in_end) const {}

}  // namespace doodle::maya_plug
