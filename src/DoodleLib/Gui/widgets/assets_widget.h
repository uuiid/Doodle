//
// Created by TD on 2021/9/16.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/base_windwos.h>

#include <boost/signals2.hpp>
namespace doodle {

class DOODLELIB_API assets_widget : public metadata_widget {
  MetadataPtr p_root;
  void load_meta(const MetadataPtr& in_ptr);

 public:
  assets_widget();
  void frame_render() override;

  void set_metadata(const MetadataPtr& in_ptr);

  void set_select(const MetadataPtr& in_ptr);
  MetadataPtr p_meta;
  boost::signals2::signal<void(const MetadataPtr&)> select_change;
};
}  // namespace doodle
