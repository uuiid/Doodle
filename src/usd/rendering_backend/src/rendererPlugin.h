//
// Created by TD on 2024/4/2.
//

#pragma once
#include "pxr/imaging/hd/rendererPlugin.h"
#include "pxr/pxr.h"
namespace doodle::usd {
class HdDoodleRendererPlugin final : public pxr::HdRendererPlugin {
 public:
  HdDoodleRendererPlugin()          = default;
  virtual ~HdDoodleRendererPlugin() = default;

  virtual pxr::HdRenderDelegate *CreateRenderDelegate() override;
  virtual pxr::HdRenderDelegate *CreateRenderDelegate(pxr::HdRenderSettingsMap const &settingsMap) override;

  virtual void DeleteRenderDelegate(pxr::HdRenderDelegate *renderDelegate) override;

  virtual bool IsSupported(bool gpuEnabled = true) const override;

 private:
  HdDoodleRendererPlugin(const HdDoodleRendererPlugin &)            = delete;
  HdDoodleRendererPlugin &operator=(const HdDoodleRendererPlugin &) = delete;
};
}  // namespace doodle::usd