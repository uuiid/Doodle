//
// Created by TD on 2021/9/16.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/base_windwos.h>

#include <boost/signals2.hpp>
namespace doodle {
namespace details {
class DOODLELIB_API table_column {
 public:
  table_column() : p_name(), p_render(), p_width(0){};
  string p_name;
  std::uint32_t p_width;
  std::function<bool(const assets_file_ptr& )> p_render;
  virtual void frame_render(const assets_file_ptr& in_ptr);
};
using table_column_ptr = std::shared_ptr<table_column>;
}  // namespace details

class DOODLELIB_API assets_file_widgets : public metadata_widget {
  metadata_ptr p_root;
  assets_file_ptr p_current_select;

  std::vector<details::table_column_ptr> p_colum_list;

  bool add_colum_render();

 public:
  assets_file_widgets();
  virtual void frame_render() override;
  /**
   * 
   */
  void set_metadata(const metadata_ptr& in_ptr);

  boost::signals2::signal<void(const metadata_ptr&)> select_change;
};
}  // namespace doodle
