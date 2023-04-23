//
// Created by TD on 2021/11/15.
//

#include "hud_render_node.h"

#include <doodle_core/metadata/user.h>

#include <main/maya_plug_fwd.h>
#include <maya_plug/data/maya_camera.h>
#include <maya_plug/data/play_blast.h>

#include <maya/MAnimControl.h>
#include <maya/MFnCamera.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MTextureManager.h>

namespace doodle::maya_plug {
MTypeId doodle_info_node::doodle_id{0x0005002B};
MString doodle_info_node::drawDbClassification{"drawdb/geometry/doodle_info_node"};
MString doodle_info_node::drawRegistrantId{"doodle_info_node_draw_override"};

doodle_info_node::doodle_info_node() {}

doodle_info_node::~doodle_info_node() = default;
void* doodle_info_node::creator() { return new doodle_info_node{}; }

MStatus doodle_info_node::initialize() { return MStatus::kSuccess; }

doodle_info_node_data::doodle_info_node_data() : MUserData(false) {}

doodle_info_node_data::~doodle_info_node_data() = default;

doodle_info_node_draw_override::doodle_info_node_draw_override(const MObject& obj) : MPxDrawOverride(obj, nullptr) {}

MPxDrawOverride* doodle_info_node_draw_override::Creator(const MObject& obj) {
  return new doodle_info_node_draw_override(obj);
}

doodle_info_node_draw_override::~doodle_info_node_draw_override() = default;

MHWRender::DrawAPI doodle_info_node_draw_override::supportedDrawAPIs() const {
  // this plugin supports both GL and DX
  return (MHWRender::DrawAPI::kAllDevices);
  // return (MHWRender::DrawAPI::kOpenGL |
  //         MHWRender::DrawAPI::kDirectX11 |
  //         MHWRender::DrawAPI::kOpenGLCoreProfile);
}

bool doodle_info_node_draw_override::isBounded(const MDagPath& objPath, const MDagPath& cameraPath) const {
  return false;
}

MBoundingBox doodle_info_node_draw_override::boundingBox(const MDagPath& objPath, const MDagPath& cameraPath) const {
  return MBoundingBox{};
}

MUserData* doodle_info_node_draw_override::prepareForDraw(
    const MDagPath& objPath, const MDagPath& cameraPath, const MFrameContext& frameContext, MUserData* oldData
) {
  // MStatus k_s{};
  // MFnDependencyNode k_fn_node{};

  // k_s = k_fn_node.setObject(objPath.node());
  // if (!k_s)
  //   std::cout << k_s << std::endl;
  // k_s = k_fn_node.setIcon("D:/tmp/test.png");
  // if (!k_s)
  //   std::cout << k_s << std::endl;

  return oldData;
}

void doodle_info_node_draw_override::addUIDrawables(
    const MDagPath& objPath, MHWRender::MUIDrawManager& drawManager, const MHWRender::MFrameContext& frameContext,
    const MUserData* data
) {
  // auto this_data = dynamic_cast<const doodle_info_node_data*>(data);

  // if (!this_data)
  //   return;
  // MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
  // if (!renderer)
  //   return;

  // MHWRender::MTextureManager* shaderMgr = renderer->getTextureManager();

  // MTextureArguments k_tex_load{"D:/tmp/test.png"};
  // auto k_tex = shaderMgr->acquireTexture(k_tex_load);
  // if (!k_tex)
  //   std::cout << false << std::endl;
  // drawManager.setTexture(k_tex);
  // MStringArray k_names{};
  // drawManager.getIconNames(k_names);
  // std::cout << k_names << std::endl;
  MStatus k_s{};
  MDagPath k_cam{};
  if (auto k_view = M3dView::active3dView(&k_s); k_s && k_view.isVisible()) {
    k_s = k_view.getCamera(k_cam);
    DOODLE_MAYA_CHICK(k_s);
  } else {
    if (g_reg()->ctx().contains<maya_camera>())
      k_cam = g_reg()->ctx().get<maya_camera>().p_path;
    else
      return;
  }

  static std::int32_t s_font_size{20};
  std::int32_t s_font_size_{13};

  // Start draw UI
  drawManager.beginDrawable();
  // Set font color
  drawManager.setColor(MColor(0.1f, 0.9f, 0.01f));
  // Set font size
  drawManager.setFontSize(s_font_size);

  // Draw renderer name
  int x = 0, y = 0, w = 0, h = 0;
  frameContext.getViewportDimensions(x, y, w, h);
  MColor k_color{0.2f, 0.2f, 0.2f, 0.5f};

  {
    /// 绘制相机名称
    MFnDagNode k_cam_node{k_cam.transform()};
    auto k_str            = k_cam_node.name(&k_s);
    std::int32_t k_size[] = {(std::int32_t)k_str.numChars() * s_font_size_, 30};
    drawManager.text2d(MPoint(w * 0.1f, h * 0.91f), k_str, MHWRender::MUIDrawManager::kLeft, k_size, &k_color);
  }

  {
    /// 绘制当前帧和总帧数
    auto k_len   = MAnimControl::maxTime() - MAnimControl::minTime() + 1;
    auto k_curr  = MAnimControl::currentTime();
    auto _k_time = fmt::format("{}/{}", k_curr.as(MTime::uiUnit()), k_len.as(MTime::uiUnit()));
    MString k_time{_k_time.c_str()};
    std::int32_t k_size[] = {(std::int32_t)k_time.numChars() * s_font_size_, 30};
    drawManager.text2d(MPoint(w * 0.5f, h * 0.91f), k_time, MHWRender::MUIDrawManager::kCenter, k_size, &k_color);
  }

  /// 绘制摄像机avo
  {
    MFnCamera k_fn_cam{k_cam};
    auto k_f = k_fn_cam.focalLength(&k_s);
    DOODLE_MAYA_CHICK(k_s);

    auto _k_s_ = fmt::format("FOV: {:.3f}", k_f);
    MString k_str{_k_s_.c_str()};
    std::int32_t k_size[] = {(std::int32_t)k_str.numChars() * s_font_size_, 30};
    drawManager.text2d(MPoint(w * 0.91f, h * 0.91f), k_str, MHWRender::MUIDrawManager::kRight, k_size, &k_color);
  }

  {
    /// 拍屏日期
    auto k_time = chrono::floor<chrono::minutes>(chrono::system_clock::now());
    auto _k_s_  = fmt::format("{}", k_time);
    MString k_str{_k_s_.c_str()};
    std::int32_t k_size[] = {(std::int32_t)k_str.numChars() * s_font_size_, 30};
    drawManager.text2d(MPoint(w * 0.1f, h * 0.1f), k_str, MHWRender::MUIDrawManager::kCenter, k_size, &k_color);
  }

  /// 制作人姓名
  {
    auto _k_s_ = fmt::format("{}", g_reg()->ctx().get<user::current_user>().user_name_attr());
    MString k_str{};
    k_str.setUTF8(_k_s_.c_str());
    std::int32_t k_size[] = {(std::int32_t)k_str.numChars() * s_font_size_ * 2, 35};
    drawManager.text2d(MPoint(w * 0.5f, h * 0.1f), k_str, MHWRender::MUIDrawManager::kCenter, k_size, &k_color);
  }

  drawManager.endDrawable();
}

}  // namespace doodle::maya_plug
