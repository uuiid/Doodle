//
// Created by TD on 2021/6/2.
//

#include <DoodleLib/Metadata/Model/ProjectManage.h>
#include <DoodleLib/Metadata/Project.h>
#include <DoodleLib/core/CoreSet.h>

#include <boost/numeric/conversion/cast.hpp>

#define DOLE_CHECK(item, value) \
  if (!(item).IsOk()) return value;

namespace doodle {
ProjectManage::ProjectManage()
    : wxDataViewModel(),
      p_project(CoreSet::getSet().GetMetadataSet().getAllProjects()),
      p_project_new(),
      p_project_remove() {
}

std::uint32_t ProjectManage::GetColumnCount() const {
  return 3;
}

wxString ProjectManage::GetColumnType(std::uint32_t col) const {
  return "string";
}

void ProjectManage::GetValue(wxVariant& variant, const wxDataViewItem& item, std::uint32_t col) const {
  variant = ConvStr<wxString>("None");
  DOLE_CHECK(item, )

  auto k_prj = reinterpret_cast<Project*>(item.GetID());
  switch (col) {
    case 0:
      variant = ConvStr<wxString>(k_prj->showStr());
      break;

    case 1:
      variant = ConvStr<wxString>(k_prj->str());
      break;

    case 2:
      variant = ConvStr<wxString>(k_prj->getPath());
      break;

    default:
      break;
  }
}

bool ProjectManage::SetValue(const wxVariant& variant, const wxDataViewItem& item, std::uint32_t col) {
  DOLE_CHECK(item, false);

  auto str   = ConvStr<std::string>(variant.GetString());
  auto k_prj = reinterpret_cast<Project*>(item.GetID());

  switch (col) {
    case 0: {
      k_prj->setName(str);
      ValueChanged(item, 0);
      ValueChanged(item, 1);
    } break;

    case 1:
      return false;
      break;

    case 2: {
      k_prj->setPath(str);
      ValueChanged(item, 2);
      break;
    }
    default:
      break;
  }
  return true;
}

bool ProjectManage::GetAttr(const wxDataViewItem& in_item, std::uint32_t in_col, wxDataViewItemAttr& attr) const {
  if (!in_item.IsOk())
    return false;

  auto& prj = MetadataSet::Get().Project_();
  if (!prj)
    return false;



  auto k_prj = reinterpret_cast<Project*>(in_item.GetID());
  if (*prj == *k_prj) {
    attr.SetBackgroundColour(wxColour{200, 75, 49});
    return true;
  } else
    return false;
}

std::uint32_t ProjectManage::GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const {
  if (item.IsOk())
    return 0;

  for (const auto& k_i : p_project) {
    children.Add(wxDataViewItem{k_i.get()});
  };
  return p_project.size();
}

void ProjectManage::addProject(const ProjectPtr& prj) {
  p_project_new.push_back(prj);
  p_project.push_back(prj);

  ItemAdded({}, wxDataViewItem{prj.get()});
}

bool ProjectManage::removeProject(const ProjectPtr& prj) {
  auto it   = std::find(p_project.begin(), p_project.end(), prj);
  auto it_n = std::find(p_project_new.begin(), p_project_new.end(), prj);
  if (it != p_project.end()) {
    p_project_remove.push_back(prj);
    p_project.erase(it);
    if (it_n != p_project_new.end())
      p_project_new.erase(it_n);
    ItemDeleted({}, wxDataViewItem{(*it).get()});

    return true;
  } else if (it_n != p_project_new.end()) {
    p_project_new.erase(it_n);
    ItemDeleted({}, wxDataViewItem{(*it).get()});

    return true;
  }

  else
    return false;
}

void ProjectManage::submit(const MetadataFactoryPtr& in_factory) {
  auto& set = MetadataSet::Get();
  for (const auto& prj : p_project_new) {
    prj->updata_db(in_factory);
    set.installProject(prj);
  }
  for (const auto& prj : p_project_remove) {
    prj->deleteData(in_factory);
    set.deleteProject(prj.get());
  }
}
}  // namespace doodle

#undef DOLE_CHECK
