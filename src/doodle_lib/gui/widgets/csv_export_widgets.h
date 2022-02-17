//
// Created by TD on 2022/2/17.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
namespace gui {

class DOODLELIB_API csv_export_widgets : public process_t<csv_export_widgets> {
  class impl;
  std::unique_ptr<impl> p_i;

  /**
   * @brief Get the user next time object 获取下一次人物提交时的实体文件
   *
   * @param in
   * @return entt::handle
   */
  entt::handle get_user_next_time(const entt::handle& in);
  /**
   * @brief 导出单行使用的函数
   *
   * @param in 传入的一行实体
   * @return std::string 返回的一行
   */
  std::string to_csv_line(const entt::handle& in);
  /**
   * @brief 导出单张表使用的函数
   *
   * @param in_list
   */
  void export_csv(const std::vector<entt::handle>& in_list,
                  const FSys::path& in_export_file_path);

 public:
  csv_export_widgets();
  ~csv_export_widgets();

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(const delta_type&, void* data);
};

}  // namespace gui
}  // namespace doodle
