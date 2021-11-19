//
// Created by TD on 2021/11/16.
//

#include "comm_play_blast.h"

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/exception/exception.h>
#include <doodle_lib/file_warp/image_sequence.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/thread_pool/long_term.h>
#include <maya/M3dView.h>
#include <maya/MAnimControl.h>
#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MViewport2Renderer.h>
namespace doodle::maya_plug {
string comm_play_blast::p_post_render_notification_name{"doodle_lib_maya_notification_name"};

void comm_play_blast::captureCallback(MHWRender::MDrawContext& context, void* clientData) {
  MStatus k_s{};
  auto self     = static_cast<comm_play_blast*>(clientData);
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

comm_play_blast::comm_play_blast()
    : command_base(),
      p_save_path(core_set::getSet().get_cache_root("maya_play_blast").generic_string()) {
  p_show_str = make_imgui_name(this,
                               "保存路径",
                               "拍摄",
                               "创建",
                               "选择相机");
}
FSys::path comm_play_blast::get_file_dir() {
  auto k_cache_path = core_set::getSet().get_cache_root("maya_play_blast/tmp");
  k_cache_path /= p_uuid.substr(0, 3);
  if (!FSys::exists(k_cache_path)) {
    FSys::create_directories(k_cache_path);
  }
  return k_cache_path;
}
FSys::path comm_play_blast::get_file_path(const MTime& in_time) {
  auto k_cache_path = get_file_dir();
  k_cache_path /= fmt::format("{}_{}.png", p_uuid, in_time.as(MTime::uiUnit()));
  return k_cache_path;
}

FSys::path comm_play_blast::get_out_path() const {
  auto k_cache_path = core_set::getSet().get_cache_root("maya_play_blast/tmp");
  k_cache_path /= (p_uuid + ".mp4");
  return k_cache_path;
}

MStatus comm_play_blast::play_blast(const MTime& in_start, const MTime& in_end) {
  MStatus k_s{};
  MSelectionList k_select{};

  k_select.add(p_camera_path);
  if (k_select.isEmpty())
    throw doodle_error{"没有相机可供拍摄"};
  if (p_save_path.empty())
    throw doodle_error{"输出路径为空"};

  auto k_view = M3dView::active3dView(&k_s);
  CHECK_MSTATUS_AND_RETURN_IT(k_s);

  MDagPath k_camera_path{};
  k_s = k_select.getDagPath(0, k_camera_path);
  CHECK_MSTATUS_AND_RETURN_IT(k_s);

  k_s = k_view.setCamera(k_camera_path);
  CHECK_MSTATUS_AND_RETURN_IT(k_s);

  auto k_render = MHWRender::MRenderer::theRenderer();
  k_render->addNotification(&comm_play_blast::captureCallback,
                            p_post_render_notification_name.c_str(),
                            MPassContext::kEndSceneRenderSemantic,
                            (void*)this);
  k_render->setOutputTargetOverrideSize(1920, 1280);
  k_render->setPresentOnScreen(false);
  {
    ///  开始后播放拍屏，并输出文件
    for (p_current_time = in_start;
         p_current_time <= in_end;
         ++p_current_time) {
      MAnimControl::setCurrentTime(p_current_time);
      k_view.refresh(true, true);
    }
  }
  k_render->removeNotification(p_post_render_notification_name.c_str(),
                               MPassContext::kEndSceneRenderSemantic);
  k_render->setPresentOnScreen(true);
  k_render->unsetOutputTargetOverrideSize();

  auto k_f = get_file_dir();

  image_sequence k_image{};
  k_image.set_path(k_f);
  k_image.set_out_path(get_out_path());
  auto k_ptr = new_object<long_term>();
  k_ptr->sig_progress.connect([](rational_int in_rational_int) {
    MString k_str{};
    k_str.setUTF8(fmt::format("合成拍屏进度 : {}", boost::rational_cast<std::int32_t>(in_rational_int)).c_str());
    MGlobal::displayInfo(k_str);
  });
  k_ptr->sig_finished.connect([this]() {
    MString k_str{};
    k_str.setUTF8(fmt::format("完成拍屏合成 : {}", get_out_path()).c_str());
    MGlobal::displayInfo(k_str);
  });
  k_image.create_video(k_ptr);
  return k_s;
}
bool comm_play_blast::init() {
  return true;
}

bool comm_play_blast::render() {
  if (imgui::Button(p_show_str["创建"].c_str())) {
    p_uuid = core_set::getSet().get_uuid_str();
    play_blast(MAnimControl::minTime(), MAnimControl::maxTime());
  }

  dear::Combo{p_show_str["选择相机"].c_str(), p_camera_path.asUTF8()} && [&]() {
    MStatus k_s;
    MItDag k_it{MItDag::kBreadthFirst, MFn::kCamera, &k_s};
    CHECK_MSTATUS_AND_RETURN(k_s, false);
    for (; !k_it.isDone(); k_it.next()) {
      MDagPath k_path;
      k_s = k_it.getPath(k_path);
      CHECK_MSTATUS_AND_RETURN(k_s, false);

      auto k_obj_tran = k_path.transform(&k_s);
      CHECK_MSTATUS_AND_RETURN(k_s, false);
      MFnDagNode k_node{k_obj_tran, &k_s};
      CHECK_MSTATUS_AND_RETURN(k_s, false);

      auto k_name = k_node.absoluteName(&k_s);
      CHECK_MSTATUS_AND_RETURN(k_s, false);
      auto k_u8        = k_name.asUTF8();
      auto k_is_select = (k_name == p_camera_path);
      if (imgui::Selectable(k_u8, k_is_select)) {
        p_camera_path = k_u8;
      }
      if (k_is_select)
        ImGui::SetItemDefaultFocus();
    }
    return true;
  };
  imgui::InputText(
      p_show_str["保存路径"].c_str(),
      &p_save_path);
  return false;
}

}  // namespace doodle::maya_plug
