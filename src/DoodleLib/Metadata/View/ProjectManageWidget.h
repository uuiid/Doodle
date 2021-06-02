//
// Created by TD on 2021/6/1.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {
class ProjectManage;
class DOODLELIB_API ProjectManageWidget : public wxFrame {
  wxObjectDataPtr<ProjectManage> p_model;

  MetadataFactoryPtr p_factory;
  void contextMenu(wxDataViewEvent& in_event);
  public:
  ProjectManageWidget(wxWindow* in_parent,MetadataFactoryPtr in_factory);
};
}  // namespace doodle
