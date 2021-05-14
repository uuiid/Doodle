//
// Created by TD on 2021/4/29.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <wx/dataview.h>
class wxDataViewCtrl;
namespace doodle {
class MetadataWidget : public wxFrame {
  ProjectPtr p_project_ptr_;
  MetadataFactoryPtr p_metadata_flctory_ptr_;
  wxWindowIDRef p_tree_id_;
  wxWindowIDRef p_List_id_;

  wxDataViewCtrl* p_tree_view_ctrl_;
  wxDataViewCtrl* p_list_view_ctrl_;

  void CreateProject();
  void AddProject();
  void deleteProject();

  void treeContextMenu(wxDataViewEvent& in_event);
  void listContextMenu(wxDataViewEvent& in_event);

 public:
  explicit MetadataWidget(wxWindow* in_window,wxWindowID in_id);


};

}
