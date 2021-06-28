//
// Created by TD on 2021/6/28.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/action/action.h>

namespace doodle {

class DOODLELIB_API assfile_create_action : public action {
 public:
  explicit assfile_create_action(std::any&& in_any);
  virtual void run(const MetadataPtr& in_data) override;
};

class DOODLELIB_API assfile_add_com_action : public action {
 public:
  explicit assfile_add_com_action(std::any&& in_any);
  virtual void run(const MetadataPtr& in_data) override;
};

class DOODLELIB_API assfile_datetime_action : public action {
 public:
  explicit assfile_datetime_action(std::any&& in_any);
  virtual void run(const MetadataPtr& in_data) override;
};
class DOODLELIB_API assfile_delete_action : public action {
 public:
  explicit assfile_delete_action(std::any&& in_any);
  virtual void run(const MetadataPtr& in_data) override;
};

}  // namespace doodle
