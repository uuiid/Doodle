//
// Created by TD on 2021/9/16.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/base_windwos.h>

#include <boost/signals2.hpp>
namespace doodle {

class DOODLELIB_API attribute_widgets : public base_widget {
  MetadataPtr p_root;
  AssetsFilePtr p_current_select;

 public:
  attribute_widgets();
  virtual void frame_render() override;

  void set_metadata(const MetadataPtr& in_ptr);
};
}  // namespace doodle
