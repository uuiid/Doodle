//
// Created by TD on 2021/11/15.
//

#include "hud_render_node.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MTextureManager.h>

namespace doodle {
MTypeId doodle_info_node::doodle_id{0x0008002B};
MString doodle_info_node::drawDbClassification{"drawdb/geometry/doodle_info_node"};
MString doodle_info_node::drawRegistrantId{"doodle_info_node_draw_override"};

doodle_info_node::doodle_info_node() {
}

doodle_info_node::~doodle_info_node() = default;
void* doodle_info_node::creator() {
  return new doodle_info_node{};
}

MStatus doodle_info_node::initialize() {
  return MStatus::kSuccess;
}

doodle_info_node_data::doodle_info_node_data()
    : MUserData(false) {
}

doodle_info_node_data::~doodle_info_node_data() = default;

doodle_info_node_draw_override::doodle_info_node_draw_override(const MObject& obj)
    : MPxDrawOverride(obj, nullptr) {
}

MPxDrawOverride* doodle_info_node_draw_override::Creator(const MObject& obj) {
  return new doodle_info_node_draw_override(obj);
}

doodle_info_node_draw_override::~doodle_info_node_draw_override() = default;

MHWRender::DrawAPI doodle_info_node_draw_override::supportedDrawAPIs() const {
  // this plugin supports both GL and DX
  return (MHWRender::DrawAPI::kOpenGL |
          MHWRender::DrawAPI::kDirectX11 |
          MHWRender::DrawAPI::kOpenGLCoreProfile);
}

bool doodle_info_node_draw_override::isBounded(
    const MDagPath& objPath,
    const MDagPath& cameraPath) const {
  return false;
}

MBoundingBox doodle_info_node_draw_override::boundingBox(
    const MDagPath& objPath,
    const MDagPath& cameraPath) const {
  return MBoundingBox{};
}

MUserData* doodle_info_node_draw_override::prepareForDraw(
    const MDagPath& objPath,
    const MDagPath& cameraPath,
    const MFrameContext& frameContext,
    MUserData* oldData) {
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
    const MDagPath& objPath,
    MHWRender::MUIDrawManager& drawManager,
    const MHWRender::MFrameContext& frameContext,
    const MUserData* data) {
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

  // Start draw UI
  drawManager.beginDrawable();
  // Set font color
  drawManager.setColor(MColor(0.455f, 0.212f, 0.596f));
  // Set font size
  drawManager.setFontSize(20);

  // Draw renderer name
  int x = 0, y = 0, w = 0, h = 0;
  frameContext.getViewportDimensions(x, y, w, h);
  MString k_str{"Renderer Override Options Tester"};
  std::int32_t k_size[] = {(std::int32_t)k_str.numChars() * 20, 30};
  MColor k_color{0.2f, 0.2f, 0.2f, 0.5f};
  drawManager.text2d(
      MPoint(w * 0.5f, h * 0.91f),
      k_str,
      MHWRender::MUIDrawManager::kCenter,
      k_size,
      &k_color);

  // Draw viewport information
  MString viewportInfoText("Viewport information: x= ");
  viewportInfoText += x;
  viewportInfoText += ", y= ";
  viewportInfoText += y;
  viewportInfoText += ", w= ";
  viewportInfoText += w;
  viewportInfoText += ", h= ";
  viewportInfoText += h;
  drawManager.text2d(
      MPoint(w * 0.5f, h * 0.85f),
      viewportInfoText,
      MHWRender::MUIDrawManager::kCenter);
  // drawManager.setTexture(nullptr);
  drawManager.endDrawable();
}

}  // namespace doodle