#pragma once

#include "doodle_core/configure/static_value.h"

#include "doodle_app/gui/base/base_window.h"

#include <boost/filesystem/path.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace doodle::gui {

class work_hour_filling : public base_windows<dear::Begin, work_hour_filling> {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

  /// @todo 这个原先需要使用注册表获取,现在改为使用客户端查询
  void list_time(std::int32_t in_y, std::int32_t in_m);
  /// @todo 被修改的gui数据使用客户端发送到服务器
  void modify_item(std::size_t in_index);

  void export_table(const FSys::path& in_path);

 public:
  work_hour_filling();
  ~work_hour_filling() override;

  constexpr static std::string_view name{config::menu_w::work_hour_filling};

  void init();
  [[nodiscard]] const std::string& title() const override;
  void render();

  void show_advanced_setting(bool in_);
  std::int32_t flags() override;
  void set_attr();
};

}  // namespace doodle::gui