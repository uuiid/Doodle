//
// Created by TD on 2021/11/22.
//

#include "play_blast.h"

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/exception/exception.h>
#include <doodle_lib/file_warp/image_sequence.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/thread_pool/long_term.h>
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
#include <maya_plug/command/create_hud_node.h>
namespace doodle::maya_plug {

bool camera_filter::camera::operator<(const camera_filter::camera& in_rhs) const {
  return priority < in_rhs.priority;
}
bool camera_filter::camera::operator>(const camera_filter::camera& in_rhs) const {
  return in_rhs < *this;
}
bool camera_filter::camera::operator<=(const camera_filter::camera& in_rhs) const {
  return !(in_rhs < *this);
}
bool camera_filter::camera::operator>=(const camera_filter::camera& in_rhs) const {
  return !(*this < in_rhs);
}

bool camera_filter::chick_cam(MDagPath& in_path) {
  MStatus k_s{};
  string k_path{in_path.fullPathName(&k_s).asUTF8()};
  CHECK_MSTATUS_AND_RETURN(k_s, false);
  const static std::vector reg_list{
      regex_priority_pair{std::regex{"(front|persp|side|top|camera)"}, -1000},
      regex_priority_pair{std::regex{R"(ep\d+_sc\d+)", std::regex::icase}, 30},
      regex_priority_pair{std::regex{R"(ep\d+)", std::regex::icase}, 10},
      regex_priority_pair{std::regex{R"(sc\d+)", std::regex::icase}, 10},
      regex_priority_pair{std::regex{R"(ep_\d+_sc_\d+)", std::regex::icase}, 10},
      regex_priority_pair{std::regex{R"(ep_\d+)", std::regex::icase}, 5},
      regex_priority_pair{std::regex{R"(sc_\d+)", std::regex::icase}, 5},
      regex_priority_pair{std::regex{R"(^[A-Z]+_)"}, 2},
      regex_priority_pair{std::regex{R"(_\d+_\d+)", std::regex::icase}, 2}};

  camera k_cam{in_path.node(), 0};

  for (const auto& k_reg : reg_list) {
    if (std::regex_search(k_path, k_reg.reg)) {
      k_cam.priority += k_reg.priority;
    }
  }
  p_list.push_back(std::move(k_cam));
  return true;
}

camera_filter::camera_filter()
    : p_list() {
}
MObject camera_filter::get() const {
  if (p_list.empty())
    return MObject::kNullObj;
  auto& k_obj = p_list.front();
  if (k_obj.priority > 0)
    return k_obj.p_dag_path;
  else
    return MObject::kNullObj;
}
bool camera_filter::conjecture() {
  MStatus k_s;
  MItDag k_it{MItDag::kBreadthFirst, MFn::kCamera, &k_s};
  CHECK_MSTATUS_AND_RETURN(k_s, false);
  p_list.clear();
  for (; !k_it.isDone(); k_it.next()) {
    MDagPath k_path{};
    k_s = k_it.getPath(k_path);
    CHECK_MSTATUS_AND_RETURN(k_s, false);
    chick_cam(k_path);
  }
  std::sort(p_list.begin(), p_list.end(), [](auto& in_l, auto& in_r) {
    return in_l > in_r;
  });
  return true;
}

MStatus camera_filter::set_render_cam(const MObject& in_obj) {
  MStatus k_s;
  MItDag k_it{MItDag::kBreadthFirst, MFn::kCamera, &k_s};
  CHECK_MSTATUS_AND_RETURN_IT(k_s);
  for (; !k_it.isDone(); k_it.next()) {
    MDagPath k_path{};
    k_s = k_it.getPath(k_path);
    CHECK_MSTATUS_AND_RETURN_IT(k_s);

    MFnDagNode k_cam_fn{k_path, &k_s};
    CHECK_MSTATUS_AND_RETURN_IT(k_s);

    auto k_plug = k_cam_fn.findPlug("renderable", &k_s);
    CHECK_MSTATUS_AND_RETURN_IT(k_s);
    k_plug.setBool(k_cam_fn.object() == in_obj);
  }
  return k_s;
}

string play_blast::p_post_render_notification_name{"doodle_lib_maya_notification_name"};

void play_blast::captureCallback(MHWRender::MDrawContext& context, void* clientData) {
  MStatus k_s{};
  auto self     = static_cast<play_blast*>(clientData);
  auto k_render = MHWRender::MRenderer::theRenderer();
  if (!self || !k_render) return;

  auto k_p = self->get_file_path(self->p_current_time);

  MString k_path{};
  k_path.setUTF8(k_p.generic_string().c_str());

  if (k_path.length() == 0)
    return;
  auto k_texManager = k_render->getTextureManager();
  auto k_tex        = context.copyCurrentColorRenderTargetToTexture();
  if (k_tex) {
    k_s = k_texManager->saveTexture(k_tex, k_path);
    k_texManager->releaseTexture(k_tex);
  }

  MString k_tmp{};
  if (!k_s) {
    k_tmp = k_s.errorString();
    k_tmp += " error path -> ";
    MGlobal::displayError(k_tmp + k_path);
  } else {
    k_tmp.setUTF8("捕获的颜色渲染目标到: ");
    MGlobal::displayInfo(k_tmp + k_path);
  }
}

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
  camera_filter k_f{};
  k_f.conjecture();
  auto k_c = k_f.get();
  if (k_c.isNull()) {
    MString k_str{};
    k_str.setUTF8("无法推测相机， 没有符合要求的相机");
    MGlobal::displayError(k_str);
    throw doodle_error{"无法推测相机， 没有符合要求的相机"};
  }

  MFnDagNode k_path{k_c};
  set_camera(k_path.fullPathName());
  return true;
}

MStatus play_blast::play_blast_(const MTime& in_start, const MTime& in_end) {
  p_uuid = core_set::getSet().get_uuid_str();
  MStatus k_s{};
  MSelectionList k_select{};

  k_select.add(p_camera_path);
  if (k_select.isEmpty()) {
    MString k_str{};
    k_str.setUTF8("没有相机可供拍摄");
    MGlobal::displayError(k_str);
    throw doodle_error{"没有相机可供拍摄"};
  }
  if (p_save_path.empty()) {
    MString k_str{};
    k_str.setUTF8("输出路径为空");
    MGlobal::displayError(k_str);
    throw doodle_error{"输出路径为空"};
  }

  MDagPath k_camera_path{};
  k_s = k_select.getDagPath(0, k_camera_path);
  CHECK_MSTATUS_AND_RETURN_IT(k_s);

  struct play_blast_guard {
    play_blast_guard() {
      create_hud_node k_node{};
      k_node.hide(false);
    }
    ~play_blast_guard() {
      create_hud_node k_node{};
      k_node.hide(true);
    }
  };
  play_blast_guard k_play_blast_guard{};

  MFnCamera k_cam_fn{k_camera_path, &k_s};
  CHECK_MSTATUS_AND_RETURN_IT(k_s);
  k_s = k_cam_fn.setFilmFit(MFnCamera::FilmFit::kFillFilmFit);
  CHECK_MSTATUS_AND_RETURN_IT(k_s);
  k_s = k_cam_fn.setDisplayFilmGate(false);
  CHECK_MSTATUS_AND_RETURN_IT(k_s);
  k_s = k_cam_fn.setDisplayGateMask(false);
  CHECK_MSTATUS_AND_RETURN_IT(k_s);
  auto k_displayResolution = k_cam_fn.findPlug("displayResolution", true, &k_s);
  CHECK_MSTATUS_AND_RETURN_IT(k_s);
  k_s = k_displayResolution.setBool(false);
  CHECK_MSTATUS_AND_RETURN_IT(k_s);
  k_s = camera_filter::set_render_cam(k_cam_fn.object());
  CHECK_MSTATUS_AND_RETURN_IT(k_s);

  k_s = MGlobal::executeCommand(R"(colorManagementPrefs -e -outputTransformEnabled true -outputTarget "renderer";
  colorManagementPrefs -e -outputUseViewTransform -outputTarget "renderer";)");
  CHECK_MSTATUS_AND_RETURN_IT(k_s);

  auto k_view = M3dView::active3dView(&k_s);
  if (k_s) {
    k_s = k_view.setCamera(k_camera_path);
    CHECK_MSTATUS(k_s);
    if (!k_s) {
      MGlobal::displayError("not set cam view");
    }
  } else {
    MGlobal::displayError("not find view");
  }

  //   auto k_render = MHWRender::MRenderer::theRenderer();
  //   k_s           = k_render->addNotification(&play_blast::captureCallback,
  //                                             p_post_render_notification_name.c_str(),
  //                                             MPassContext::kEndSceneRenderSemantic,
  //                                             (void*)this);
  //   CHECK_MSTATUS_AND_RETURN_IT(k_s);
  //   k_render->setOutputTargetOverrideSize(1920, 1280);
  //   k_render->setPresentOnScreen(false);
  //   {
  //     auto k_target_manager = k_render->getRenderTargetManager();
  //     auto k_target         = k_target_manager->acquireRenderTarget(
  //                 MRenderTargetDescription{
  //             "doodle",
  //             1920,
  //             1280,
  //             1,
  //             MHWRender::kR8G8B8A8_UINT,
  //             1,
  //             false});

  //     auto _k_cam_str = fmt::format("batch:{}", MFnDagNode{k_camera_path.transform()}.name().asUTF8());
  //     MString k_cam_str{};
  //     k_cam_str.setUTF8(_k_cam_str.c_str());
  //     MGlobal::displayInfo(k_cam_str);
  //     ///  开始后播放拍屏，并输出文件
  //     for (p_current_time = in_start;
  //          p_current_time <= in_end;
  //          ++p_current_time) {
  //       k_s = MAnimControl::setCurrentTime(p_current_time);
  //       CHECK_MSTATUS_AND_RETURN_IT(k_s);

  //       auto k_r = k_render->render(k_cam_str, &k_target, 1);
  //       if (!k_r)
  //         MGlobal::displayError("not render");

  //       // k_s = k_view.refresh(false, true);
  //     }
  //     k_target_manager->releaseRenderTarget(k_target);
  //   }
  //   k_s = k_render->removeNotification(p_post_render_notification_name.c_str(),
  //                                      MPassContext::kEndSceneRenderSemantic);
  //   CHECK_MSTATUS_AND_RETURN_IT(k_s);

  //   k_render->setPresentOnScreen(true);
  //   k_render->unsetOutputTargetOverrideSize();

  auto k_mel = fmt::format(R"(playblast 
-compression "png" 
-filename "{}" 
-format "image" 
-height 1280 
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
    auto k_len = MAnimControl::maxTime() - MAnimControl::minTime();
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
    k_w.set_text(fmt::format("FOV: {:.3f}", k_cam_fn.focalLength()));
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
    MString k_str{};
    k_str.setUTF8(fmt::format("合成拍屏进度 : {}%", k_ptr->get_progress_int()).c_str());
    MGlobal::displayInfo(k_str);
  });
  k_ptr->sig_finished.connect([this]() {
    MString k_str{};
    k_str.setUTF8(fmt::format("完成拍屏合成 : {}", get_out_path()).c_str());
    MGlobal::displayInfo(k_str);
    FSys::remove_all(get_file_dir());
  });
  k_image.create_video(k_ptr);
  // k_view.scheduleRefresh();
  return k_s;
}

bool play_blast::conjecture_ep_sc() {
  FSys::path p_current_path{MFileIO::currentFile().asUTF8()};
  auto k_r = p_eps.analysis(p_current_path) &&
             p_shot.analysis(p_current_path);
  set_save_filename(p_current_path.filename().replace_extension(".mp4"));
  return k_r;
}

}  // namespace doodle::maya_plug