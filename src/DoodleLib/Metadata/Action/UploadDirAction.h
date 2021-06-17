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

  virtual void run() override;
};

}  // namespace doodle
