//
// Created by TD on 2021/5/6.
//

#include <DoodleLib/Metadata/Model/AssDirTree.h>
#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/core/coreset.h>

#include <DoodleLib/FileWarp/MayaFile.h>
#include <DoodleLib/FileWarp/ImageSequence.h>
#include <DoodleLib/FileWarp/VideoSequence.h>
#include <DoodleLib/FileWarp/Ue4Project.h>

#include <DoodleLib/FileSys/FileSystem.h>

#include <DoodleLib/Metadata/Project.h>
#include <DoodleLib/Metadata/Episodes.h>
#include <DoodleLib/Metadata/Shot.h>

#define DOLE_CHECK(item, value) \
  if (!(item).IsOk()) return value;
namespace doodle {
AssDirTree::AssDirTree()
    : wxDataViewModel(),
      p_Root(MetadataSet::Get().getAllProjects()) {

}

unsigned int AssDirTree::GetColumnCount() const {
  return 1;
}

wxString AssDirTree::GetColumnType(unsigned int col) const {
  return "string";
}

void AssDirTree::GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const {
  variant = ConvStr<wxString>("None");
  DOLE_CHECK(item, );
  auto str = reinterpret_cast<Metadata*>(item.GetID());
  variant  = ConvStr<wxString>(str->showStr());
}

bool AssDirTree::SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) {
  return false;
}

wxDataViewItem AssDirTree::GetParent(const wxDataViewItem& item) const {
  DOLE_CHECK(item, wxDataViewItem{});
  const auto k_p_metadata = reinterpret_cast<Metadata*>(item.GetID());
  if (k_p_metadata->hasChild())
    return wxDataViewItem{k_p_metadata->getParent().get()};
  else
    return wxDataViewItem{};
}

bool AssDirTree::IsContainer(const wxDataViewItem& item) const {
  DOLE_CHECK(item, true);

  auto k_item = reinterpret_cast<Metadata*>(item.GetID());
  return !k_item->hasChild();
}

unsigned int AssDirTree::GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const {
  //  DOLE_CHECK(item, 0);

  auto k_item = reinterpret_cast<Metadata*>(item.GetID());
  if (!item.IsOk()) {
    //这里是空指针的情况， 即没有父级， 我们要使用根来确认
    for (const auto& k_t : p_Root) {
      children.Add(wxDataViewItem{k_t.get()});
    }
  } else {
    const auto& k_child = k_item->getChildItems();
    for (const auto& k_t : k_child) {
      children.Add(wxDataViewItem{k_t.get()});
    }
  }
  return children.size();
}
}  // namespace doodle
