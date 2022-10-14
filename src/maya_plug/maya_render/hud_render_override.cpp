//
// Created by TD on 2021/11/15.
//

#include "hud_render_override.h"
namespace doodle {
hud_render_override::hud_render_override(const MString &in_name)
    : MRenderOverride(in_name),
      mUIName("doodle_hud"),
      mOperations({nullptr, nullptr}),
      mOperationNames({"hud_render_override_hud_render", "hud_render_override_present"}),
      mCurrentOperation(-1) {
}
hud_render_override::~hud_render_override() = default;

// Drawing uses all internal code so will support all draw APIs
//
MHWRender::DrawAPI hud_render_override::supportedDrawAPIs() const {
  return MHWRender::kAllDevices;
}
bool hud_render_override::startOperationIterator() {
  mCurrentOperation = 0;
  return true;
}

MHWRender::MRenderOperation *
hud_render_override::renderOperation() {
  if (mCurrentOperation >= 0 && mCurrentOperation < 2) {
    if (mOperations[mCurrentOperation]) {
      return mOperations[mCurrentOperation].get();
    }
  }
  return nullptr;
}

bool hud_render_override::nextRenderOperation() {
  mCurrentOperation++;
  if (mCurrentOperation < 2) {
    return true;
  }
  return false;
}

MStatus hud_render_override::cleanup() {
  mCurrentOperation = -1;
  return MStatus::kSuccess;
}

MStatus hud_render_override::setup(const MString &destination) {
  MHWRender::MRenderer *theRenderer = MHWRender::MRenderer::theRenderer();
  if (!theRenderer)
    return MStatus::kFailure;

  // Create a new set of operations as required
  if (!mOperations[0]) {
    mOperations[0] = std::make_unique<hud_render>(mOperationNames[0]);
    mOperations[1] = std::make_unique<MHWRender::MPresentTarget>(mOperationNames[1]);
  }
  if (!mOperations[0] ||
      !mOperations[1]) {
    return MStatus::kFailure;
  }

  return MStatus::kSuccess;
}
hud_render::hud_render(const MString &in_name)
    : MUserRenderOperation(in_name) {
  mName = in_name;
}

bool hud_render::hasUIDrawables() const {
  return true;
}
void hud_render::addUIDrawables(
    MHWRender::MUIDrawManager &drawManager2D,
    const MHWRender::MFrameContext &frameContext
) {
  // Start draw UI
  drawManager2D.beginDrawable();
  // Set font color
  drawManager2D.setColor(MColor(0.455f, 0.212f, 0.596f));
  // Set font size
  drawManager2D.setFontSize(MHWRender::MUIDrawManager::kSmallFontSize);

  // Draw renderer name
  int x = 0, y = 0, w = 0, h = 0;
  frameContext.getViewportDimensions(x, y, w, h);
  drawManager2D.text(
      MPoint(w * 0.5f, h * 0.91f),
      MString("Renderer Override Options Tester"),
      MHWRender::MUIDrawManager::kCenter
  );

  // Draw viewport information
  MString viewportInfoText("Viewport information: x= ");
  viewportInfoText += x;
  viewportInfoText += ", y= ";
  viewportInfoText += y;
  viewportInfoText += ", w= ";
  viewportInfoText += w;
  viewportInfoText += ", h= ";
  viewportInfoText += h;
  drawManager2D.text(
      MPoint(w * 0.5f, h * 0.885f),
      viewportInfoText,
      MHWRender::MUIDrawManager::kCenter
  );
  drawManager2D.endDrawable();
}
const MFloatPoint *hud_render::viewportRectangleOverride() {
  mViewRectangle[0] = 0.25f;
  mViewRectangle[1] = 0.25f;
  mViewRectangle[2] = 0.75f;
  mViewRectangle[3] = 0.75f;
  return &mViewRectangle;
}
// bool hud_render::getInputTargetDescription(
//     const MString &name,
//     MRenderTargetDescription &description) {
//   return MHUDRender::getInputTargetDescription(name, description);
// }

// MRenderTarget *const *hud_render::targetOverrideList(
//     unsigned int &listSize) {
//   return MHUDRender::targetOverrideList(listSize);
// }
}  // namespace doodle
