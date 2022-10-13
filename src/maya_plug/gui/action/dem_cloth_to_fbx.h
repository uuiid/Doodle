//
// Created by TD on 2022/7/4.
//

#pragma

#include <doodle_app/gui/base/base_window.h>

#include <main/maya_plug_fwd.h>
#include <maya_plug/configure/static_value.h>

namespace doodle::maya_plug {

class dem_cloth_to_fbx
    : public doodle::gui::base_windows<dear::Begin, dem_cloth_to_fbx> {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  constexpr static auto name = ::doodle::gui::config::maya_plug::menu::dem_cloth_to_fbx;
  dem_cloth_to_fbx();
  ~dem_cloth_to_fbx() override;
  virtual void init();
  void render();
  const std::string& title() const override;
};

}  // namespace doodle::maya_plug
