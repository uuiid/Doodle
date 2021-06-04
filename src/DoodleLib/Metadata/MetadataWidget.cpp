//
// Created by TD on 2021/4/29.
//

#include <DoodleLib/DoodleApp.h>
#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/FileSys/FileSystem.h>
#include <DoodleLib/FileWarp/ImageSequence.h>
#include <DoodleLib/FileWarp/MayaFile.h>
#include <DoodleLib/FileWarp/Ue4Project.h>
#include <DoodleLib/FileWarp/VideoSequence.h>
#include <DoodleLib/Metadata/ContextMenu.h>
#include <DoodleLib/Metadata/Episodes.h>
#include <DoodleLib/Metadata/MetadataFactory.h>
#include <DoodleLib/Metadata/MetadataWidget.h>
#include <DoodleLib/Metadata/Model/AssetsTree.h>
#include <DoodleLib/Metadata/Project.h>
#include <DoodleLib/Metadata/Shot.h>
#include <DoodleLib/core/CoreSet.h>
#include <doodlelib/Metadata/Model/ProjectManage.h>
namespace doodle {

MetadataWidget::MetadataWidget(wxWindow* in_window, wxWindowID in_id)
    : wxFrame(in_window, in_id, ConvStr<wxString>("Metadata")),
      p_project_ptr_(),
      p_metadata_flctory_ptr_(std::make_shared<MetadataFactory>()),
      p_tree_id_(NewControlId()),
      p_List_id_(NewControlId()),
      p_tree_view_ctrl_(new wxDataViewCtrl{this, p_tree_id_}),
      p_list_view_ctrl_(),
      p_project_view_ctrl_(new wxDataViewCtrl{this, NewControlId()}),
      p_project_model(new ProjectManage{}),
      p_assstsTree_model(new AssetsTree{}) {
  auto k_layout = new wxBoxSizer{wxOrientation::wxVERTICAL};

  p_project_view_ctrl_->AssociateModel(p_project_model.get());
  p_project_view_ctrl_->AppendTextColumn(
      ConvStr<wxString>("名称"),
      0,
      wxDataViewCellMode::wxDATAVIEW_CELL_EDITABLE);
  p_project_view_ctrl_->AppendTextColumn(
      ConvStr<wxString>("拼音名称"),
      1,
      wxDataViewCellMode::wxDATAVIEW_CELL_EDITABLE);
  p_project_view_ctrl_->AppendTextColumn(
      ConvStr<wxString>("路径"),
      2,
      wxDataViewCellMode::wxDATAVIEW_CELL_EDITABLE);
  k_layout->Add(p_project_view_ctrl_, wxSizerFlags{1}.Expand().Border(0));

  /// 绑定各种函数
  p_project_view_ctrl_->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &MetadataWidget::projectContextMenu, this);

  auto k_layout_1 = new wxBoxSizer{wxOrientation::wxHORIZONTAL};
  k_layout->Add(k_layout_1, wxSizerFlags{6}.Expand().Border(wxDirection::wxALL, 0));

  // p_list_view_ctrl_ = new wxDataViewCtrl{this, p_List_id_};
  p_tree_view_ctrl_->AppendTextColumn(
      ConvStr<wxString>("标签树"), 0,
      wxDataViewCellMode::wxDATAVIEW_CELL_INERT);
  p_tree_view_ctrl_->SetMinSize(wxSize{300, 600});
  p_tree_view_ctrl_->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &MetadataWidget::treeContextMenu, this);

  k_layout_1->Add(p_tree_view_ctrl_, wxSizerFlags{2}.Expand().Border(wxDirection::wxALL, 0));

  p_tree_view_ctrl_->Bind(
      wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,
      &MetadataWidget::treeContextMenu,
      this);

  // auto k_p_text_renderer = new wxDataViewTextRenderer{"string", wxDATAVIEW_CELL_EDITABLE};
  // auto k_col             = new wxDataViewColumn{ConvStr<wxString>("标签树"), k_p_text_renderer, 0, 100};
  // p_tree_view_ctrl_->AppendColumn(k_col);
  // p_tree_view_ctrl_->AssociateModel(new AssetsTree{});
  // k_p_text_renderer = new wxDataViewTextRenderer{"string", wxDATAVIEW_CELL_EDITABLE};
  // k_col             = new wxDataViewColumn{ConvStr<wxString>("文件"), k_p_text_renderer, 0, 100};
  // p_list_view_ctrl_->AppendColumn(k_col);
  // p_list_view_ctrl_->SetMinSize(wxSize{300, 600});

  SetSizer(k_layout);
  k_layout->SetSizeHints(this);
  this->Center();
}

void MetadataWidget::treeContextMenu(wxDataViewEvent& in_event) {
  wxMenu k_wx_menu{};
  ContextMenu k_context_menu{this, &k_wx_menu};
  auto k_data = in_event.GetItem();
  if (k_data.IsOk()) {
    auto k_item = reinterpret_cast<Metadata*>(k_data.GetID());
    k_item->createMenu(&k_context_menu);
  } else {
    k_context_menu.createMenuAfter();
  }
  PopupMenu(&k_wx_menu);
}
void MetadataWidget::listContextMenu(wxDataViewEvent& in_event) {
}

void MetadataWidget::projectContextMenu(wxDataViewEvent& in_event) {
  wxMenu k_wx_menu{};

  auto k_set_prj = k_wx_menu.AppendCheckItem(NewControlId(), ConvStr<wxString>("设置当前项目"));
  k_wx_menu.AppendSeparator();

  auto k_add_prj    = k_wx_menu.AppendCheckItem(NewControlId(), ConvStr<wxString>("添加项目"));
  auto k_remove_prj = k_wx_menu.AppendCheckItem(NewControlId(), ConvStr<wxString>("删除项目"));
  auto k_sub_prj    = k_wx_menu.AppendCheckItem(NewControlId(), ConvStr<wxString>("提交项目"));

  k_wx_menu.Bind(
      wxEVT_MENU, [this, &in_event](wxCommandEvent& in_event_menu) {
        if (!p_project_view_ctrl_->HasSelection())
          return;
        auto k_i = in_event.GetItem();
        if (!k_i.IsOk())
          return;
        auto k_p = reinterpret_cast<Project*>(k_i.GetID());
        if (!k_p)
          return;
        MetadataSet::Get().setProject_(k_p);
        p_assstsTree_model->setRoot(std::dynamic_pointer_cast<Project>(k_p->shared_from_this()));
      },
      k_set_prj->GetId());
  k_wx_menu.Bind(
      wxEVT_MENU, [this, &in_event](wxCommandEvent& in_event_menu) {
        this->p_project_model->addProject(std::make_shared<Project>("C:/", "none"));
      },
      k_add_prj->GetId());
  k_wx_menu.Bind(
      wxEVT_MENU, [this, &in_event](wxCommandEvent& in_event_menu) {
        if (!p_project_view_ctrl_->HasSelection())
          return;

        auto k_item = in_event.GetItem();
        if (!k_item.IsOk())
          return;
        auto sele  = reinterpret_cast<Project*>(k_item.GetID());
        auto k_prj = std::dynamic_pointer_cast<Project>(sele->shared_from_this());
        this->p_project_model->removeProject(k_prj);
      },
      k_remove_prj->GetId());
  k_wx_menu.Bind(
      wxEVT_MENU, [this, &in_event](wxCommandEvent& in_event_menu) {
        p_project_model->submit(this->p_metadata_flctory_ptr_);
      },
      k_sub_prj->GetId());

  PopupMenu(&k_wx_menu);
}

}  // namespace doodle
