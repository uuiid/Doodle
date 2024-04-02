//
// Created by TD on 2024/4/2.
//

#include "rendererPlugin.h"

#include "pxr/imaging/hd/rendererPluginRegistry.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/pxr.h"
namespace pxr {
TF_REGISTRY_FUNCTION(pxr::TfType) { pxr::HdRendererPluginRegistry::Define<doodle::usd::HdStormRendererPlugin>(); }
}  // namespace pxr
namespace doodle::usd {
pxr::HdRenderDelegate* HdStormRendererPlugin::CreateRenderDelegate() { return new pxr::HdStRenderDelegate(); }

pxr::HdRenderDelegate* HdStormRendererPlugin::CreateRenderDelegate(pxr::HdRenderSettingsMap const& settingsMap) {
  return new pxr::HdStRenderDelegate(settingsMap);
}

void HdStormRendererPlugin::DeleteRenderDelegate(pxr::HdRenderDelegate* renderDelegate) { delete renderDelegate; }

bool HdStormRendererPlugin::IsSupported(bool gpuEnabled) const {
  return gpuEnabled ? pxr::HdStRenderDelegate::IsSupported() : false;
}
}  // namespace doodle::usd