//
// Created by TD on 2021/5/14.
//

#include <DoodleLib/Logger/Logger.h>
#include <DoodleLib/Metadata/Assets.h>
#include <DoodleLib/Metadata/AssetsFile.h>
#include <DoodleLib/Metadata/AssetsPath.h>
#include <DoodleLib/Metadata/ContextMenu.h>
#include <DoodleLib/Metadata/Episodes.h>
#include <DoodleLib/Metadata/Metadata.h>
#include <DoodleLib/Metadata/MetadataFactory.h>
#include <DoodleLib/Metadata/Metadata_cpp.h>
#include <DoodleLib/Metadata/Model/AssetsTree.h>
#include <DoodleLib/Metadata/Model/ListAttributeModel.h>
#include <DoodleLib/Metadata/Model/ProjectManage.h>
#include <DoodleLib/Metadata/Project.h>
#include <DoodleLib/Metadata/Shot.h>
#include <DoodleLib/Metadata/View/TimeWidget.h>
#include <FileWarp/MayaFile.h>
#include <FileWarp/Ue4Project.h>
#include <wx/dataview.h>
#include <wx/numdlg.h>
#include <wx/textdlg.h>

namespace doodle {

ContextMenu::ContextMenu(wxWindow* in_parent, wxMenu* in_menu, wxDataViewModel* in_model)
    : p_parent(in_parent),
      p_menu(in_menu),
      p_metadata_flctory_ptr_(std::make_shared<MetadataFactory>()),
      p_model(in_model) {
}
ProjectPtr ContextMenu::createProject() {
  auto path_dialog = wxDirDialog{p_parent, ConvStr<wxString>("选择项目根目录: ")};  //, wxEmptyString, wxRESIZE_BORDER
  auto result      = path_dialog.ShowModal();
  if (result != wxID_OK)
    return {};
  auto k_text_dialog = wxTextEntryDialog{p_parent, ConvStr<wxString>("项目名称: ")};
  auto k_result      = k_text_dialog.ShowModal();
  if (k_result != wxID_OK)
    return {};

  auto k_path = ConvStr<std::string>(path_dialog.GetPath());
  auto k_name = ConvStr<std::string>(k_text_dialog.GetValue());
  if (k_path.empty() || k_name.empty())
    return {};

  auto k_ptr = std::make_shared<Project>(k_path, k_name);
  return k_ptr;
}

wxMenu* ContextMenu::createMenu(const ProjectPtr& in_data) {
  auto k_add_prj    = p_menu->AppendCheckItem(wxWindow::NewControlId(), ConvStr<wxString>("添加项目"));
  auto k_remove_prj = p_menu->AppendCheckItem(wxWindow::NewControlId(), ConvStr<wxString>("删除项目"));
  auto k_sub_prj    = p_menu->AppendCheckItem(wxWindow::NewControlId(), ConvStr<wxString>("提交项目"));

  p_menu->Bind(
      wxEVT_MENU, [this, in_data](wxCommandEvent& in_event_menu) {
        auto k_model = dynamic_cast<ProjectManage*>(p_model);
        if (k_model) {
          auto prj = createProject();
          if (prj)
            k_model->addProject(prj);
        }
      },
      k_add_prj->GetId());
  p_menu->Bind(
      wxEVT_MENU, [this, in_data](wxCommandEvent& in_event_menu) {
        auto k_model = dynamic_cast<ProjectManage*>(p_model);
        if (k_model)
          k_model->removeProject(in_data);
      },
      k_remove_prj->GetId());
  p_menu->Bind(
      wxEVT_MENU, [this, in_data](wxCommandEvent& in_event_menu) {
        auto k_model = dynamic_cast<ProjectManage*>(p_model);
        if (k_model)
          k_model->submit(this->p_metadata_flctory_ptr_);
      },
      k_sub_prj->GetId());

  return this->createMenuAfter(std::dynamic_pointer_cast<Metadata>(in_data));
}

wxMenu* ContextMenu::createMenu(const EpisodesPtr& in_data) {
  auto k_eps = p_menu->Append(wxID_ANY, ConvStr<wxString>("修改集数"));
  p_menu->Bind(
      wxEVT_MENU,
      [this, in_data](wxCommandEvent& in_event) {
        auto k_num = wxGetNumberFromUser(ConvStr<wxString>("输入集数"),
                                         ConvStr<wxString>(""),
                                         ConvStr<wxString>("集数"),
                                         1, 0, 9999, p_parent);
        in_data->setEpisodes(k_num);
        in_data->updata_db(this->p_metadata_flctory_ptr_);
      },
      k_eps->GetId());

  return this->createMenuAfter(std::dynamic_pointer_cast<Metadata>(in_data));
}

wxMenu* ContextMenu::createMenu(const ShotPtr& in_data) {
  //  auto k_DeleteShot = p_menu->Append(wxID_ANY, ConvStr<wxString>("删除镜头"));

  auto k_add_shotab = p_menu->Append(wxID_ANY, ConvStr("添加ab镜头"));
  p_menu->AppendSeparator();
  auto k_modify_shot   = p_menu->Append(wxID_ANY, ConvStr<wxString>("修改镜头号"));
  auto k_modify_shotab = p_menu->Append(wxID_ANY, ConvStr("修改ab镜头"));

  p_menu->Bind(
      wxEVT_MENU,
      [this, in_data](wxCommandEvent& in_event) {
        auto k_shotAb = getShotAb();
        auto k_p      = in_data->getParent();
        auto k_r      = k_p->addChildItem(
            std::make_shared<Shot>(k_p, in_data->getShot(), k_shotAb));
        k_r->updata_db(p_metadata_flctory_ptr_);
      },
      k_add_shotab->GetId());
  p_menu->Bind(
      wxEVT_MENU,
      [this, in_data](wxCommandEvent& in_event) {
        auto shot = wxGetNumberFromUser(ConvStr<wxString>("输入镜头"),
                                        ConvStr<wxString>(""),
                                        ConvStr<wxString>("镜头"),
                                        1, 0, 9999, p_parent);
        in_data->setShot(shot);
        in_data->updata_db(this->p_metadata_flctory_ptr_);
      },
      k_modify_shot->GetId());
  p_menu->Bind(
      wxEVT_MENU,
      [this, in_data](wxCommandEvent& in_event) {
        auto k_shotAb = getShotAb();
        in_data->setShotAb(k_shotAb);
        in_data->updata_db(this->p_metadata_flctory_ptr_);
      },
      k_modify_shotab->GetId());

  return this->createMenuAfter(std::dynamic_pointer_cast<Metadata>(in_data));
}
std::string ContextMenu::getShotAb() const {
  wxArrayString k_array_string{};
  for (const auto& str : magic_enum::enum_names<Shot::ShotAbEnum>()) {
    k_array_string.emplace_back(std::string{str});
  }
  auto k_shotAb_wxstring = wxGetSingleChoice(
      ConvStr<wxString>("ab镜头"),
      ConvStr<wxString>("ab镜头"),
      k_array_string,
      0, p_parent);
  auto k_shotAb = ConvStr<std::string>(k_shotAb_wxstring);
  return k_shotAb;
}

wxMenu* ContextMenu::createMenu(const AssetsPtr& in_data) {
  auto k_ass = p_menu->Append(wxID_ANY, ConvStr<wxString>("修改名称"));
  p_menu->Bind(
      wxEVT_MENU,
      [this, in_data](wxCommandEvent& in_event) {
        auto text = wxGetTextFromUser(ConvStr<wxString>("类别名称"),
                                      ConvStr<wxString>("类别名称"),
                                      ConvStr<wxString>("none"), p_parent);
        if (text.empty())
          return;
        in_data->setName1(text);
        in_data->updata_db(this->p_metadata_flctory_ptr_);
      },
      k_ass->GetId());
  return this->createMenuAfter(std::dynamic_pointer_cast<Metadata>(in_data));
}

wxMenu* ContextMenu::createMenu(const AssetsFilePtr& in_data) {
  // auto k_path = wxGetTextFromUser(ConvStr<wxString>("类别名称"),
  //                                 ConvStr<wxString>("类别名称"),
  //                                 ConvStr<wxString>("none"), p_parent);
  auto k_time = p_menu->Append(wxID_ANY, ConvStr<wxString>("修改时间"));
  auto k_com  = p_menu->Append(wxID_ANY, ConvStr<wxString>("添加评论"));

  p_menu->Bind(
      wxEVT_MENU,
      [in_data, this](wxCommandEvent& in_event) {
        auto k_time = TimeWidget::get_time(p_parent, in_data->getTime());
        if (k_time) {
          in_data->setTime(k_time);
          in_data->updata_db(this->p_metadata_flctory_ptr_);
        }
      },
      k_time->GetId());
  p_menu->Bind(
      wxEVT_MENU, [in_data, this](wxCommandEvent& in_event) {
        auto k_str = wxGetTextFromUser(ConvStr<wxString>("评论"),
                                       ConvStr<wxString>("评论: "),
                                       ConvStr<wxString>("none"), p_parent);
        if (!k_str.empty()) {
          in_data->addComment(std::make_shared<Comment>(ConvStr<std::string>(k_str)));
          in_data->updata_db(this->p_metadata_flctory_ptr_);
        }
      },
      k_com->GetId());

  auto k_path = in_data->getPathFile().begin()->getLocalPath();
  if (MayaFile::is_maya_file(k_path)) {
    p_menu->Append(wxID_ANY, ConvStr<wxString>("导出相机"));
  }

  if (Ue4Project::can_import_ue4(k_path)) {
    p_menu->Append(wxID_ANY, ConvStr<wxString>("导入ue4"));
  }

  return this->createMenuAfter(std::dynamic_pointer_cast<Metadata>(in_data));
}

wxMenu* ContextMenu::createMenuAfter(const MetadataPtr& in_data) {
  if (!dynamic_cast<AssetsTree*>(p_model))
    return p_menu;

  p_menu->AppendSeparator();

  auto k_add_eps  = p_menu->Append(wxID_ANY, ConvStr<wxString>("添加集数"));
  auto k_add_shot = p_menu->Append(wxID_ANY, ConvStr<wxString>("添加镜头"));
  auto k_add_ass  = p_menu->Append(wxID_ANY, ConvStr<wxString>("添加类别"));

  auto k_delete = p_menu->Append(wxID_ANY, ConvStr<wxString>("删除"));

  p_menu->Bind(
      wxEVT_MENU, [in_data, this](wxCommandEvent& in_event) {
        auto eps = wxGetNumberFromUser(ConvStr<wxString>("输入集数"),
                                       ConvStr<wxString>(""),
                                       ConvStr<wxString>("集数"),
                                       1, 0, 9999, p_parent);
        auto k_r = in_data->addChildItem(std::make_shared<Episodes>(in_data, eps));
        k_r->updata_db(p_metadata_flctory_ptr_);
      },
      k_add_eps->GetId());
  p_menu->Bind(
      wxEVT_MENU, [in_data, this](wxCommandEvent& in_event) {
        auto shot = wxGetNumberFromUser(ConvStr<wxString>("输入镜头"),
                                        ConvStr<wxString>(""),
                                        ConvStr<wxString>("镜头"),
                                        1, 0, 9999, p_parent);
        auto k_r  = in_data->addChildItem(std::make_shared<Shot>(in_data, shot));
        k_r->updata_db(p_metadata_flctory_ptr_);
      },
      k_add_shot->GetId());
  p_menu->Bind(
      wxEVT_MENU, [in_data, this](wxCommandEvent& in_event) {
        auto str = wxGetTextFromUser(ConvStr<wxString>("类别名称"),
                                     ConvStr<wxString>("类别名称"),
                                     ConvStr<wxString>("none"), p_parent);
        if (str.empty())
          return;
        auto k_r = in_data->addChildItem(std::make_shared<Assets>(in_data, ConvStr<std::string>(str)));
        k_r->updata_db(p_metadata_flctory_ptr_);
      },
      k_add_ass->GetId());

  p_menu->Bind(
      wxEVT_MENU, [in_data, this](wxCommandEvent& in_event) {
        try {
          if (in_data->hasParent()) {
            if (in_data->hasChild())
              wxMessageBox(ConvStr<wxString>("有子物体，无法删除"),
                           ConvStr<wxString>("注意"), wxYES_NO | wxCANCEL, p_parent);
            else {
              auto k_p = in_data->getParent();
              ///这里必须先删除再清除子物体
              in_data->deleteData(p_metadata_flctory_ptr_);
              k_p->removeChildItems(in_data->shared_from_this());
            }
          } else {
            wxMessageBox(ConvStr<wxString>("这个是项目,无法删除"),
                         ConvStr<wxString>("注意"), wxYES | wxCANCEL, p_parent);
          };
        } catch (const std::exception& err) {
          DOODLE_LOG_DEBUG(err.what());
        }
      },
      k_delete->GetId());

  return createMenuAfter();
}
wxMenu* ContextMenu::createMenuAfter() {
  return this->p_menu;
}

}  // namespace doodle
