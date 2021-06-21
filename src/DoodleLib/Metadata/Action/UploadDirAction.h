//
// Created by TD on 2021/6/17.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Metadata/Action/Action.h>


namespace doodle {
class DOODLELIB_API UploadDirAction :public Action{


 public:
  UploadDirAction();
  explicit UploadDirAction(std::any && path);
  void run(const MetadataPtr& in_data) override;

  std::string class_name() override;
  void operator()(const MetadataPtr& in_data) override;
};

}  // namespace doodle
