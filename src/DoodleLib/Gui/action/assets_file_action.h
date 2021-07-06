//
// Created by TD on 2021/6/28.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/action/action.h>

namespace doodle {

class DOODLELIB_API actn_assfile_create : public action {
 public:
  actn_assfile_create();
  /**
   * @brief 输入的是显示名称
   * @param in_any 显示名称（std::string）
   */
  explicit actn_assfile_create(std::any&& in_any);
  void run(const MetadataPtr& in_data) override;
};

class DOODLELIB_API actn_assfile_add_com : public action {
 public:
  actn_assfile_add_com();
  explicit actn_assfile_add_com(std::any&& in_any);
  void run(const MetadataPtr& in_data) override;
};

class DOODLELIB_API actn_assfile_datetime : public action {
 public:
  actn_assfile_datetime();
  explicit actn_assfile_datetime(std::any&& in_any);
  void run(const MetadataPtr& in_data) override;
};
class DOODLELIB_API actn_assfile_delete : public action {
 public:
  actn_assfile_delete();
  explicit actn_assfile_delete(std::any&& in_any);
  void run(const MetadataPtr& in_data) override;
};

}  // namespace doodle
