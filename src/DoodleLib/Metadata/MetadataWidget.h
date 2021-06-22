//
// Created by TD on 2021/4/29.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <wx/dataview.h>
#include <boost/signals2.hpp>
class wxDataViewCtrl;

namespace doodle {


class MetadataWidget : public wxFrame {
  MetadataFactoryPtr p_metadata_flctory_ptr_;
  wxWindowIDRef p_tree_id_;
  wxWindowIDRef p_List_id_;

  wxDataViewCtrl* p_project_view_ctrl_;
  wxDataViewCtrl* p_assets_tree_view_ctrl_;
  wxDataViewCtrl* p_assets_attribute_view_ctrl_;
  wxObjectDataPtr<ProjectManage> p_project_model;
  wxObjectDataPtr<AssetsTree> p_assets_tree_model;
  wxObjectDataPtr<ListAttributeModel> p_assets_attribute_model;

  boost::signals2::connection p_conn;
  void drag_files(wxDropFilesEvent& in_event);
  void context_menu(wxDataViewEvent& in_event,wxDataViewModel* in_model);
 public:
  explicit MetadataWidget(wxWindow* in_window, wxWindowID in_id);
  virtual ~MetadataWidget();
};

}  // namespace doodle
