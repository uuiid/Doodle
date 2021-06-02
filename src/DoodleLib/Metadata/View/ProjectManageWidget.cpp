//
// Created by TD on 2021/6/1.
//

#include <DoodleLib/Metadata/View/ProjectManageWidget.h>
#include <doodlelib/Metadata/Model/ProjectManage.h>

// #include <winrt/Windows.Foundation.Collections.h>
#include <wx/dataview.h>
namespace doodle {
// void ProjectManageWidget::contextMenu(wxDataViewEvent& in_event) {
//   wxMenu k_menu{};
//   auto add_prj = k_menu.AppendCheckItem(NewControlId(), ConvStr<wxString>(""));
// }

ProjectManageWidget::ProjectManageWidget(wxWindow* in_parent, MetadataFactoryPtr in_factory)
    : wxFrame(
          in_parent,
          NewControlId(),
          ConvStr<wxString>("项目管理设置")),
      p_model(new ProjectManage{}),
      p_factory(std::move(in_factory)) {
  auto layout = new wxBoxSizer{wxOrientation::wxVERTICAL};
  auto k_ctrl = new wxDataViewCtrl{this, NewControlId()};

  k_ctrl->AssociateModel(p_model.get());
  k_ctrl->AppendTextColumn(
      ConvStr<wxString>("名称"),
      0,
      wxDataViewCellMode::wxDATAVIEW_CELL_INERT);
  k_ctrl->AppendTextColumn(
      ConvStr<wxString>("拼音名称"),
      0,
      wxDataViewCellMode::wxDATAVIEW_CELL_INERT);
  k_ctrl->AppendTextColumn(
      ConvStr<wxString>("路径"),
      0,
      wxDataViewCellMode::wxDATAVIEW_CELL_INERT);
  layout->Add(k_ctrl, wxSizerFlags{1}.Expand().Border(0));

  auto k_butten = new wxButton{this, NewControlId(), ConvStr<wxString>("提交")};
  layout->Add(k_butten, wxSizerFlags{0}.Expand().Border(0));

  SetSizer(layout);
  layout->SetSizeHints(this);
  this->Center();

//   k_ctrl->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &ProjectManageWidget::contextMenu, this);
}

}  // namespace doodle