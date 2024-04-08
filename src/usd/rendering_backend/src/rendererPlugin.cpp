//
// Created by TD on 2024/4/2.
//

#include "rendererPlugin.h"

#include "pxr/imaging/hd/rendererPluginRegistry.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/pxr.h"
namespace pxr {
TF_REGISTRY_FUNCTION(TfType) { pxr::HdRendererPluginRegistry::Define<doodle::usd::HdDoodleRendererPlugin>(); }
}  // namespace pxr
namespace doodle::usd {
pxr::HdRenderDelegate* HdDoodleRendererPlugin::CreateRenderDelegate() { return new pxr::HdStRenderDelegate(); }

pxr::HdRenderDelegate* HdDoodleRendererPlugin::CreateRenderDelegate(pxr::HdRenderSettingsMap const& settingsMap) {
  return new pxr::HdStRenderDelegate(settingsMap);
}

void HdDoodleRendererPlugin::DeleteRenderDelegate(pxr::HdRenderDelegate* renderDelegate) { delete renderDelegate; }

bool HdDoodleRendererPlugin::IsSupported(bool gpuEnabled) const {
  return gpuEnabled ? pxr::HdStRenderDelegate::IsSupported() : false;
}
}  // namespace doodle::usd