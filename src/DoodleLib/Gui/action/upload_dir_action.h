//
// Created by TD on 2021/6/17.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/action/action.h>

namespace doodle {
class DOODLELIB_API upload_dir_action :public action {


 public:
  upload_dir_action();
  explicit upload_dir_action(std::any && path);
  void run(const MetadataPtr& in_data) override;

  std::string class_name() override;
  void operator()(const MetadataPtr& in_data) override;
};

}  // namespace doodle
