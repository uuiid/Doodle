//
// Created by TD on 2021/11/15.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <maya/MViewport2Renderer.h>

namespace doodle {
class hud_render_override : public MHWRender::MRenderOverride {
 private:
 public:
  hud_render_override(const MString& name);
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
  std::array<MHWRender::MRenderOperation*, 2> mOperations;
  std::array<MString, 2> mOperationNames;

  // Temporary of operation iteration
  int mCurrentOperation;
};

class hud_render : public MHWRender::MHUDRender {
 public:
  hud_render(const MString& name);
};

}  // namespace doodle