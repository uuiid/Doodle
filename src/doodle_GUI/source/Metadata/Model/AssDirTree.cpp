//
// Created by TD on 2021/5/6.
//

#include <doodle_GUI/source/Metadata/Model/AssDirTree.h>
#include <corelib/core_Cpp.h>

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
  variant = ConvStr<wxString>(str->ShowStr());
}

bool AssDirTree::SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) {
  return false;
}

wxDataViewItem AssDirTree::GetParent(const wxDataViewItem& item) const {
  DOLE_CHECK(item, wxDataViewItem{});
  const auto k_p_metadata = reinterpret_cast<Metadata*>(item.GetID());
  if (k_p_metadata->HasChild())
    return wxDataViewItem{k_p_metadata->GetPParent().get()};
  else
    return wxDataViewItem{};
}

bool AssDirTree::IsContainer(const wxDataViewItem& item) const {
  DOLE_CHECK(item, true);

  auto k_item = reinterpret_cast<Metadata*>(item.GetID());
  return !k_item->HasChild();
}

unsigned int AssDirTree::GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const {
//  DOLE_CHECK(item, 0);

  auto k_item = reinterpret_cast<Metadata*>(item.GetID());
  if (!k_item) {
    for (const auto& k_t : p_Root) {
      children.Add(wxDataViewItem{k_t.get()});
    }
  } else {
    const auto& k_child = k_item->GetPChildItems();
    for (const auto& k_t : k_child) {
      children.Add(wxDataViewItem{k_t.get()});
    }
  }
  return children.size();
}
}  // namespace doodle
