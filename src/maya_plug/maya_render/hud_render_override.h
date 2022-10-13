//
// Created by TD on 2021/11/15.
//

#pragma once

#include <array>
#include <maya/MViewport2Renderer.h>
namespace doodle {
class hud_render_override : public MHWRender::MRenderOverride {
 private:
 public:
  hud_render_override(const MString& in_name);
  ~hud_render_override();

  MHWRender::DrawAPI supportedDrawAPIs() const override;

  // Basic setup and cleanup
  MStatus setup(const MString& destination) override;
  MStatus cleanup() override;

  // Operation iteration methods
  bool startOperationIterator() override;
  MHWRender::MRenderOperation* renderOperation() override;
  bool nextRenderOperation() override;

  // UI name
  MString uiName() const override {
    return mUIName;
  }

 protected:
  // UI name
  MString mUIName;
  // Operations and operation names
  std::array<
      std::unique_ptr<MHWRender::MRenderOperation>,
      2>
      mOperations;
  std::array<MString, 2> mOperationNames;

  // Temporary of operation iteration
  int mCurrentOperation;
};

class hud_render : public MHWRender::MUserRenderOperation {
 public:
  hud_render(const MString& in_name);

  bool hasUIDrawables() const override;
  void addUIDrawables(
      MHWRender::MUIDrawManager& drawManager2D,
      const MHWRender::MFrameContext& frameContext
  ) override;
  const MFloatPoint* viewportRectangleOverride() override;

  /// 测试MUserRenderOperation所需覆盖方法
  bool requiresLightData() const override {
    return false;
  }

  MStatus execute(const MHWRender::MDrawContext& drawContext) override {
    return MStatus::kSuccess;
  }
  // bool getInputTargetDescription(
  //     const MString& name,
  //     MRenderTargetDescription& description) override;
  // MRenderTarget* const* targetOverrideList(
  //     unsigned int& listSize) override;

 private:
  MFloatPoint mViewRectangle;
};

}  // namespace doodle
