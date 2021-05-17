//
// Created by TD on 2021/5/14.
//

#include <DoodleLib/Metadata/ContextMenu.h>
#include <DoodleLib/Metadata/Metadata.h>
#include <DoodleLib/Metadata/Project.h>
#include <DoodleLib/Metadata/Episodes.h>
#include <DoodleLib/Metadata/Shot.h>
#include <DoodleLib/Metadata/Assets.h>
#include <DoodleLib/Metadata/AssetsFile.h>

#include <DoodleLib/core/MetadataSet.h>
#include <DoodleLib/Metadata/MetadataFactory.h>
namespace doodle{

ContextMenu::ContextMenu(wxWindow* in_parent,wxMenu* in_menu)
  :p_parent(in_parent),
      p_menu(in_menu),
   p_metadata_flctory_ptr_(std::make_shared<MetadataFactory>()){
}
void ContextMenu::CreateProject()  {
  auto path_dialog = wxDirDialog{p_parent, ConvStr<wxString>("选择项目根目录: "), wxEmptyString, wxRESIZE_BORDER};
  auto result      = path_dialog.ShowModal();
  if (result != wxID_OK)
    return;
  auto k_text_dialog = wxTextEntryDialog{p_parent, ConvStr<wxString>("项目名称: ")};
  auto k_result      = k_text_dialog.ShowModal();
  if (k_result != wxID_OK)
    return;

  auto k_path = ConvStr<FSys::path>(path_dialog.GetPath());
  auto k_name = ConvStr<std::string>(k_text_dialog.GetValue());
  if(k_path.empty() || k_name.empty())
    return;

  auto k_ptr = std::make_shared<Project>(k_path,k_name);
  k_ptr->save(p_metadata_flctory_ptr_);
  MetadataSet::Get().installProject(k_ptr);
}
void ContextMenu::AddProject() {
  auto path_dialog = wxDirDialog{p_parent, ConvStr<wxString>("选择项目根目录: "), wxEmptyString, wxRESIZE_BORDER};
  auto result      = path_dialog.ShowModal();
  if (result != wxID_OK) return;
  auto path = ConvStr<FSys::path>(path_dialog.GetPath());
  if (path.empty()) return;
  auto prj = std::make_shared<Project>(path);
  prj->load(p_metadata_flctory_ptr_);
  MetadataSet::Get().installProject(prj);
}

void ContextMenu::createMenu(const ProjectPtr& in_data) {
  auto k_set_prj = p_menu->Append(wxID_ANY, ConvStr<wxString>("设为当前项目"));
  p_menu->Bind(
            wxEVT_MENU,
            [in_data](wxCommandEvent& in_event){
              MetadataSet::Get().setProject_(in_data);
            },
            k_set_prj->GetId()
            );

  auto k_delete_prj = p_menu->Append(wxID_ANY, ConvStr<wxString>("删除项目"));
  p_menu->Bind(
            wxEVT_MENU,
            [in_data](wxCommandEvent& in_event){
              MetadataSet::Get().deleteProject(in_data.get());
            },
            k_set_prj->GetId()
            );


  this->createMenuAfter(std::dynamic_pointer_cast<Metadata>(in_data));
}

void ContextMenu::createMenu(const EpisodesPtr& in_data) {
  this->createMenuAfter(std::dynamic_pointer_cast<Metadata>(in_data));
}

void ContextMenu::createMenu(const ShotPtr& in_data) {
  auto k_add_shot = p_menu->Append(wxID_ANY, ConvStr<wxString>("添加镜头"));
  auto k_DeleteShot = p_menu->Append(wxID_ANY, ConvStr<wxString>("删除镜头"));
  auto k_add_shotab = p_menu->Append(wxID_ANY, ConvStr("添加ab镜头"));
  p_menu->Bind(
      wxEVT_MENU,[](wxCommandEvent&in_event){},k_add_shot->GetId()
      );
  this->createMenuAfter(std::dynamic_pointer_cast<Metadata>(in_data));
}

void ContextMenu::createMenu(const AssetsPtr& in_data) {
  this->createMenuAfter(std::dynamic_pointer_cast<Metadata>(in_data));
}

void ContextMenu::createMenu(const AssetsFilePtr& in_data) {
  this->createMenuAfter(std::dynamic_pointer_cast<Metadata>(in_data));
}

void ContextMenu::createMenuAfter(const MetadataPtr& in_data)  {
  createMenuAfter();
}
void ContextMenu::createMenuAfter() {
  auto k_create_prj = p_menu->Append(wxID_ANY, ConvStr<wxString>("创建项目"));
  p_menu->Bind(
      wxEVT_MENU,
      [this](wxCommandEvent&in_event){
        this->CreateProject();
      },
      k_create_prj->GetId()
  );
  auto k_Add_prj = p_menu->Append(wxID_ANY, ConvStr<wxString>("添加项目"));
  p_menu->Bind(
      wxEVT_MENU,
      [this](wxCommandEvent&in_event){
        this->AddProject();
      },
      k_create_prj->GetId()
  );
}

}
