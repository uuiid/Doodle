//
// Created by TD on 2021/4/29.
//

#include <DoodleLib/Metadata/MetadataWidget.h>
#include <DoodleLib/DoodleApp.h>
#include <DoodleLib/Metadata/Model/AssDirTree.h>
#include <DoodleLib/core_Cpp.h>
#include <DoodleLib/Metadata/MetadataFactory.h>
#include <DoodleLib/Metadata/ContextMenu.h>
namespace doodle {

MetadataWidget::MetadataWidget(wxWindow* in_window, wxWindowID in_id)
    : wxFrame(in_window, in_id, ConvStr<wxString>("Metadata")),
      p_project_ptr_(),
      p_metadata_flctory_ptr_(std::make_shared<MetadataFactory>()),
      p_tree_id_(NewControlId()),
      p_List_id_(NewControlId()),
      p_tree_view_ctrl_(),
      p_list_view_ctrl_() {
  auto k_layout     = new wxBoxSizer{wxHORIZONTAL};
  p_tree_view_ctrl_ = new wxDataViewCtrl{this, p_tree_id_};
  p_list_view_ctrl_ = new wxDataViewCtrl{this, p_List_id_};

  auto k_p_text_renderer = new wxDataViewTextRenderer{"string", wxDATAVIEW_CELL_EDITABLE};
  auto k_col             = new wxDataViewColumn{ConvStr<wxString>("标签树"), k_p_text_renderer, 0, 100};
  p_tree_view_ctrl_->AppendColumn(k_col);
  p_tree_view_ctrl_->AssociateModel(new AssDirTree{});
  k_p_text_renderer = new wxDataViewTextRenderer{"string", wxDATAVIEW_CELL_EDITABLE};
  k_col             = new wxDataViewColumn{ConvStr<wxString>("文件"), k_p_text_renderer, 0, 100};
  p_list_view_ctrl_->AppendColumn(k_col);
  p_tree_view_ctrl_->SetMinSize(wxSize{300, 600});
  p_list_view_ctrl_->SetMinSize(wxSize{300, 600});

  k_layout->Add(p_tree_view_ctrl_, wxSizerFlags{0}.Expand().Border(wxALL, 0))->SetProportion(2);
  k_layout->Add(p_list_view_ctrl_, wxSizerFlags{0}.Expand().Border(wxALL, 0))->SetProportion(3);

  //绑定各种信号
  p_tree_view_ctrl_->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,&MetadataWidget::treeContextMenu,this);
  p_list_view_ctrl_->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,&MetadataWidget::listContextMenu,this);
  //关闭时隐藏，不销毁
  Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
    this->Hide();
    if (event.CanVeto())
      event.Veto(false);
  });

  SetSizer(k_layout);
  k_layout->SetSizeHints(this);
  this->Center();
}

void MetadataWidget::treeContextMenu(wxDataViewEvent& in_event) {
  wxMenu k_wx_menu{};
  ContextMenu k_context_menu{this,&k_wx_menu};
  auto k_data = in_event.GetItem();
  if(k_data.IsOk()){
    auto k_item = reinterpret_cast<Metadata*>(k_data.GetID());
    k_item->createMenu(&k_context_menu);
  } else{
    k_context_menu.createMenuAfter();
  }
  PopupMenu(&k_wx_menu);
}
void MetadataWidget::listContextMenu(wxDataViewEvent& in_event) {

}

}  // namespace doodle
