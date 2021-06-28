//
// Created by TD on 2021/6/17.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/action/action.h>
namespace doodle {
class upload_file_action : public action {
 public:
  upload_file_action();
  explicit upload_file_action(std::any&& in_any);

  virtual std::string class_name() override;
  virtual void run(const MetadataPtr& in_data) override;
  virtual void operator()(const MetadataPtr& in_data) override;
};

}  // namespace doodle
