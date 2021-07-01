//
// Created by TD on 2021/6/28.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/action/action.h>

namespace doodle {

class DOODLELIB_API assfile_create_action : public action {
 public:
  assfile_create_action();
  /**
   * @brief 输入的是显示名称
   * @param in_any 显示名称（std::string）
   */
  explicit assfile_create_action(std::any&& in_any);
  void run(const MetadataPtr& in_data) override;
};

class DOODLELIB_API assfile_add_com_action : public action {
 public:
  assfile_add_com_action();
  explicit assfile_add_com_action(std::any&& in_any);
  void run(const MetadataPtr& in_data) override;
};

class DOODLELIB_API assfile_datetime_action : public action {
 public:
  assfile_datetime_action();
  explicit assfile_datetime_action(std::any&& in_any);
  void run(const MetadataPtr& in_data) override;
};
class DOODLELIB_API assfile_delete_action : public action {
 public:
  assfile_delete_action();
  explicit assfile_delete_action(std::any&& in_any);
  void run(const MetadataPtr& in_data) override;
};

}  // namespace doodle
