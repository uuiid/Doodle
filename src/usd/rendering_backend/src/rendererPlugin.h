//
// Created by TD on 2024/4/2.
//

#pragma once
#include "pxr/imaging/hd/rendererPlugin.h"
#include "pxr/pxr.h"
namespace doodle::usd {
class HdStormRendererPlugin final : public pxr::HdRendererPlugin {
 public:
  HdStormRendererPlugin()          = default;
  virtual ~HdStormRendererPlugin() = default;

  virtual pxr::HdRenderDelegate *CreateRenderDelegate() override;
  virtual pxr::HdRenderDelegate *CreateRenderDelegate(pxr::HdRenderSettingsMap const &settingsMap) override;

  virtual void DeleteRenderDelegate(pxr::HdRenderDelegate *renderDelegate) override;

  virtual bool IsSupported(bool gpuEnabled = true) const override;

 private:
  HdStormRendererPlugin(const HdStormRendererPlugin &)            = delete;
  HdStormRendererPlugin &operator=(const HdStormRendererPlugin &) = delete;
};
}  // namespace doodle::usd