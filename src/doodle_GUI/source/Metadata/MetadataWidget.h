//
// Created by TD on 2021/4/29.
//

#pragma once
#include <doodle_global.h>

namespace doodle {
class MetadataWidget : public wxFrame {
  ProjectPtr p_project_ptr_;
 public:
  explicit MetadataWidget();

  void CreateProject() const;

};

}
