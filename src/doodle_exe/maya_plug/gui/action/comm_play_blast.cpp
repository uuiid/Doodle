//
// Created by TD on 2021/11/16.
//

#include "comm_play_blast.h"

#include <Maya/MDrawContext.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/exception/exception.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <maya/M3dView.h>
#include <maya/MAnimControl.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MViewport2Renderer.h>
namespace doodle::maya_plug {
string comm_play_blast::p_post_render_notification_name{"doodle_lib_maya_notification_name"};
string comm_play_blast::p_post_render_notification_semantic{"doodle_lib_maya_notification_semantic"};

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
                               "拍摄");
}

FSys::path comm_play_blast::get_file_path(const MTime& in_time) {
  auto k_cache_path = core_set::getSet().get_cache_root("maya_play_blast/tmp");
  k_cache_path /= p_uuid.substr(0, 3);
  if (!FSys::exists(k_cache_path)) {
    FSys::create_directories(k_cache_path);
  }
  k_cache_path /= fmt::format("{}_{}.png", p_uuid, in_time.as(MTime::k25FPS));
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
  k_render->addNotification(&captureCallback,
                            p_post_render_notification_name.c_str(),
                            p_post_render_notification_semantic.c_str(),
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
                               p_post_render_notification_semantic.c_str());
  k_render->setPresentOnScreen(true);
  k_render->unsetOutputTargetOverrideSize();
  return k_s;
}
bool comm_play_blast::init() {
  return true;
}

bool comm_play_blast::render() {
  if (imgui::Button(p_show_str["创建"].c_str())) {
    play_blast(MAnimControl::animationStartTime(), MAnimControl::animationEndTime());
  }

  dear::Combo{p_show_str["选择相机"].c_str(), "none"} && [&]() {
    MStatus k_s;
    MItDag k_it{MItDag::kBreadthFirst, MFn::kCamera, &k_s};
    CHECK_MSTATUS_AND_RETURN(k_s, false);
    for (; !k_it.isDone(); k_it.next()) {
      MFnDagNode k_node{k_it.currentItem(&k_s)};
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