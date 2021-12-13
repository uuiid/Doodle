//
// Created by TD on 2021/11/22.
//

#include "play_blast.h"

#include "doodle_lib/core/core_set.h"
#include "doodle_lib/exception/exception.h"
#include "doodle_lib/file_warp/image_sequence.h"
#include "doodle_lib/lib_warp/imgui_warp.h"
#include "doodle_lib/thread_pool/long_term.h"
#include <maya_plug/data/maya_camera.h>

#include <fmt/chrono.h>
#include <maya/M3dView.h>
#include <maya/MAnimControl.h>
#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MFileIO.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnCamera.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MViewport2Renderer.h>
#include "create_hud_node.h"
namespace doodle::maya_plug {

string play_blast::p_post_render_notification_name{"doodle_lib_maya_notification_name"};

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
    : p_save_path(core_set::getSet().get_cache_root("maya_play_blast")),
      p_camera_path(),
      p_eps(),
      p_shot(),
      p_current_time(),
      p_uuid() {
}
FSys::path play_blast::get_file_dir() const {
  auto k_cache_path = core_set::getSet().get_cache_root("maya_play_blast/tmp");
  k_cache_path /= p_uuid.substr(0, 3);
  if (!FSys::exists(k_cache_path)) {
    FSys::create_directories(k_cache_path);
  }
  return k_cache_path;
}

FSys::path play_blast::get_file_path(const MTime& in_time) const {
  auto k_cache_path = get_file_dir();
  k_cache_path /= fmt::format("{}_{:05d}.png", p_uuid,
                              boost::numeric_cast<std::int32_t>(in_time.as(MTime::uiUnit())));
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
  p_save_path.replace_filename(in_save_filename);
  return get_out_path();
}

FSys::path play_blast::get_out_path() const {
  if (!FSys::exists(p_save_path.parent_path()))
    FSys::create_directories(p_save_path.parent_path());
  // k_cache_path /= fmt::format("{}_{}.mp4", p_eps, p_shot);
  return p_save_path;
}

void play_blast::set_camera(const MString& in_dag_path) {
  p_camera_path = in_dag_path;
}

bool play_blast::conjecture_camera() {
  auto k_cam = g_reg()->ctx_or_set<maya_camera>();
  k_cam.conjecture();
  set_camera(k_cam.p_path.fullPathName());
  return true;
}

MStatus play_blast::play_blast_(const MTime& in_start, const MTime& in_end) {
  p_uuid = core_set::getSet().get_uuid_str();
  MStatus k_s{};
  MSelectionList k_select{};

  k_select.add(p_camera_path);
  if (k_select.isEmpty()) {
    DOODLE_LOG_ERROR("没有相机可供拍摄")
    throw doodle_error{"没有相机可供拍摄"};
  }
  if (p_save_path.empty()) {
    DOODLE_LOG_ERROR("输出路径为空");
    throw doodle_error{"输出路径为空"};
  }

  MDagPath k_camera_path{};
  k_s = k_select.getDagPath(0, k_camera_path);
  CHECK_MSTATUS_AND_RETURN_IT(k_s);

  struct play_blast_guard {
    play_blast_guard() {
      create_hud_node k_node{};
      // k_node.hide(true);
    }
    ~play_blast_guard() {
      create_hud_node k_node{};
      // k_node.hide(true);
    }
  };
  play_blast_guard k_play_blast_guard{};

  try {
    chick_ctx<maya_camera>();
  } catch (const doodle_error& err) {
    g_reg()->set<maya_camera>().conjecture();
  }
  auto& k_cam = g_reg()->ctx<maya_camera>();
  k_cam.conjecture();
  k_cam.set_render_cam();
  k_cam.set_play_attr();

  k_s = MGlobal::executeCommand(R"(colorManagementPrefs -e -outputTransformEnabled true -outputTarget "renderer";
  colorManagementPrefs -e -outputUseViewTransform -outputTarget "renderer";)");
  CHECK_MSTATUS_AND_RETURN_IT(k_s);

  auto k_view = M3dView::active3dView(&k_s);
  if (k_s) {
    k_s = k_view.setCamera(k_camera_path);
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

  if (MGlobal::mayaState(&k_s) == MGlobal::kInteractive) {
    auto k_mel = fmt::format(R"(playblast 
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
                             get_out_path().replace_extension("").generic_string(),
                             in_start.as(MTime::uiUnit()),
                             in_end.as(MTime::uiUnit()));
    k_s        = MGlobal::executeCommand(k_mel.c_str());
    CHECK_MSTATUS_AND_RETURN_IT(k_s);
    return MStatus::kSuccess;
  } else {
    auto k_mel = fmt::format(R"(playblast 
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
                             get_file_path().generic_string(),
                             in_start.as(MTime::uiUnit()),
                             in_end.as(MTime::uiUnit()));
    k_s        = MGlobal::executeCommand(k_mel.c_str());
    CHECK_MSTATUS_AND_RETURN_IT(k_s);
    MGlobal::executeCommand(R"(colorManagementPrefs -e -outputTransformEnabled false -outputTarget "renderer";)");

    auto k_f = get_file_dir();

    image_sequence k_image{};
    k_image.set_path(k_f);
    k_image.set_out_path(get_out_path());
    {
      ///添加水印
      details::watermark k_w{};
      k_w.set_text(MFnDagNode{k_camera_path.transform()}.name().asUTF8());
      k_w.set_text_point(0.1, 0.1);
      k_w.set_text_color(25, 220, 2);
      k_image.add_watermark(k_w);
    }
    {
      ///添加水印
      details::watermark k_w{};
      /// 绘制当前帧和总帧数
      auto k_len = MAnimControl::maxTime() - MAnimControl::minTime() + 1;
      auto k_min = MAnimControl::minTime();
      k_w.set_text([=](std::int32_t in_frame) -> string {
        return fmt::format("{}/{}", k_min.as(MTime::uiUnit()) + in_frame, k_len.as(MTime::uiUnit()));
      });
      k_w.set_text_point(0.5, 0.1);
      k_w.set_text_color(25, 220, 2);
      k_image.add_watermark(k_w);
    }
    {
      ///添加水印
      /// 绘制摄像机avo
      details::watermark k_w{};
      k_w.set_text(fmt::format("FOV: {:.3f}", k_cam.focalLength()));
      k_w.set_text_point(0.91, 0.1);
      k_w.set_text_color(25, 220, 2);
      k_image.add_watermark(k_w);
    }
    {
      ///添加水印
      /// 绘制摄像机avo
      details::watermark k_w{};
      auto k_time = chrono::floor<chrono::minutes>(chrono::system_clock::now());
      k_w.set_text(fmt::format("{}", k_time));
      k_w.set_text_point(0.1, 0.91);
      k_w.set_text_color(25, 220, 2);
      k_image.add_watermark(k_w);
    }
    {
      {
        /// 制作人姓名
        details::watermark k_w{};
        k_w.set_text(fmt::format("{}", core_set::getSet().get_user_en()));
        k_w.set_text_point(0.5, 0.91);
        k_w.set_text_color(25, 220, 2);
        k_image.add_watermark(k_w);
      }
    }

    auto k_ptr = new_object<long_term>();
    k_ptr->sig_progress.connect([&](rational_int in_rational_int) {
      DOODLE_LOG_INFO("合成拍屏进度 : {}%", k_ptr->get_progress_int());
    });
    k_ptr->sig_finished.connect([this]() {
      DOODLE_LOG_INFO("完成拍屏合成 : {}", get_out_path());
      FSys::remove_all(get_file_dir());
    });
    k_image.create_video(k_ptr);
    // k_view.scheduleRefresh();
    return k_s;
  }
}

bool play_blast::conjecture_ep_sc() {
  FSys::path p_current_path{MFileIO::currentFile().asUTF8()};
  auto k_r = p_eps.analysis(p_current_path) &&
             p_shot.analysis(p_current_path);
  set_save_filename(p_current_path.filename().replace_extension(".mp4"));
  return k_r;
}

}  // namespace doodle::maya_plug
