//
// Created by TD on 2021/6/21.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Metadata/Action/Action.h>
namespace doodle {
class UploadDirAndFileAction : public Action {
 public:
  UploadDirAndFileAction();
  explicit UploadDirAndFileAction(std::any && in_paths);

  virtual std::string class_name() override;


  virtual void run(const MetadataPtr& in_data) override;

  virtual void operator()(const MetadataPtr& in_data) override;
};

}  // namespace doodle
