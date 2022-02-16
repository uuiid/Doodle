//
// Created by TD on 2021/9/16.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/base_windwos.h>

#include <boost/signals2.hpp>
namespace doodle {

class assets_file_widgets;

/**
 * @brief 文件列表显示
 * @image html assets_file_widgets.jpg  显示的文件
 * 文件列表显示了在资产项目下所有的文件文件
 * 在显示的文件中，路径并不是所有的， 而是最主要的一条
 * @note 每次上传文件都会递增版本号， 如果需要新的条目请创建新条目
 *
 */
class DOODLELIB_API assets_file_widgets : public process_t<assets_file_widgets> {
  class impl;
  std::unique_ptr<impl> p_i;

  void set_select(const entt::handle& in_);
  void render_context_menu(const entt::handle& in_);

 public:
  entt::handle p_current_select;

  assets_file_widgets();
  ~assets_file_widgets() override;

  constexpr static std::string_view name{"文件列表"};

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(delta_type, void* data);

  boost::signals2::signal<void(const entt::handle&)> select_change;
};

}  // namespace doodle
