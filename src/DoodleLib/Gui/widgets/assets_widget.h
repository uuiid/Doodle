//
// Created by TD on 2021/9/16.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/base_windwos.h>

#include <boost/signals2.hpp>
namespace doodle {

class DOODLELIB_API assets_widget : public metadata_widget {
  metadata_ptr p_root;
  void load_meta(const metadata_ptr& in_ptr);

 public:
  assets_widget();
  void frame_render() override;

  void set_metadata(const metadata_ptr& in_ptr);

  void set_select(const metadata_ptr& in_ptr);
  metadata_ptr p_meta;
  boost::signals2::signal<void(const metadata_ptr&)> select_change;
};
}  // namespace doodle
