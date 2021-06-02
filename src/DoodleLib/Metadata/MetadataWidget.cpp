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
#include <DoodleLib/Metadata/Model/AssstsTree.h>
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
      p_tree_view_ctrl_(),
      p_list_view_ctrl_(),
      p_project_model(new ProjectManage{}) {
  auto k_layout = new wxBoxSizer{wxOrientation::wxVERTICAL};

  auto k_ctrl = new wxDataViewCtrl{this, NewControlId()};

  k_ctrl->AssociateModel(p_project_model.get());
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
  k_layout->Add(k_ctrl, wxSizerFlags{1}.Expand().Border(0));

  auto k_butten = new wxButton{this, NewControlId(), ConvStr<wxString>("提交")};
  k_layout->Add(k_butten, wxSizerFlags{0}.Expand().Border(0));

  /// 绑定各种函数
  k_ctrl->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &MetadataWidget::projectContextMenu, this);
  // p_tree_view_ctrl_ = new wxDataViewCtrl{this, p_tree_id_};
  // p_list_view_ctrl_ = new wxDataViewCtrl{this, p_List_id_};

  // auto k_p_text_renderer = new wxDataViewTextRenderer{"string", wxDATAVIEW_CELL_EDITABLE};
  // auto k_col             = new wxDataViewColumn{ConvStr<wxString>("标签树"), k_p_text_renderer, 0, 100};
  // p_tree_view_ctrl_->AppendColumn(k_col);
  // p_tree_view_ctrl_->AssociateModel(new AssstsTree{});
  // k_p_text_renderer = new wxDataViewTextRenderer{"string", wxDATAVIEW_CELL_EDITABLE};
  // k_col             = new wxDataViewColumn{ConvStr<wxString>("文件"), k_p_text_renderer, 0, 100};
  // p_list_view_ctrl_->AppendColumn(k_col);
  // p_tree_view_ctrl_->SetMinSize(wxSize{300, 600});
  // p_list_view_ctrl_->SetMinSize(wxSize{300, 600});

  // k_layout->Add(p_tree_view_ctrl_, wxSizerFlags{0}.Expand().Border(wxALL, 0))->SetProportion(2);
  // k_layout->Add(p_list_view_ctrl_, wxSizerFlags{0}.Expand().Border(wxALL, 0))->SetProportion(3);

  // 绑定各种信号
  // p_tree_view_ctrl_->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,&MetadataWidget::treeContextMenu,this);
  // p_list_view_ctrl_->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,&MetadataWidget::listContextMenu,this);
  // //关闭时隐藏，不销毁
  // Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
  //   this->Hide();
  //   if (event.CanVeto())
  //     event.Veto(false);
  // });

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
      wxEVT_MENU, [](wxCommandEvent& in_event) {

      },
      k_set_prj->GetId());
  k_wx_menu.Bind(
      wxEVT_MENU, [](wxCommandEvent& in_event) {

      },
      k_add_prj->GetId());
  k_wx_menu.Bind(
      wxEVT_MENU, [](wxCommandEvent& in_event) {

      },
      k_remove_prj->GetId());
  k_wx_menu.Bind(
      wxEVT_MENU, [](wxCommandEvent& in_event) {

      },
      k_sub_prj->GetId());
  PopupMenu(&k_wx_menu);
}

}  // namespace doodle
