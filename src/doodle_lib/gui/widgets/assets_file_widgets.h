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
  std::function<bool(const entt::handle&)> p_render;
  virtual void frame_render(const entt::handle& in_ptr);
};
using table_column_ptr = std::shared_ptr<table_column>;
}  // namespace details

/**
 * @brief 文件列表显示
 * @image html assets_file_widgets.jpg  显示的文件
 * 文件列表显示了在资产项目下所有的文件文件
 * 在显示的文件中，路径并不是所有的， 而是最主要的一条
 * @note 每次上传文件都会递增版本号， 如果需要新的条目请创建新条目
 *
 */
class DOODLELIB_API assets_file_widgets : public metadata_widget {
  entt::handle p_root;
  entt::entity p_current_select;

  std::vector<details::table_column_ptr> p_colum_list;
  registry_ptr reg;
  bool add_colum_render();

 public:
  using list_data = boost::hana::tuple<season_ref*, episodes_ref*, shot_ref*, assets_ref*>;

  assets_file_widgets();
  virtual void frame_render() override;
  /**
   *
   */
  void set_metadata(const entt::entity& in_ptr);

  boost::signals2::signal<void(const entt::entity&)> select_change;
};
}  // namespace doodle
