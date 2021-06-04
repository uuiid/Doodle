//
// Created by TD on 2021/5/6.
//

#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/FileWarp/ImageSequence.h>
#include <DoodleLib/FileWarp/Ue4Project.h>
#include <DoodleLib/Metadata/Episodes.h>
#include <DoodleLib/Metadata/MetadataFactory.h>
#include <DoodleLib/Metadata/Model/AssstsTree.h>
#include <DoodleLib/Metadata/Project.h>
#include <DoodleLib/core/CoreSet.h>

#define DOLE_CHECK(item, value) \
  if (!(item).IsOk()) return value;
namespace doodle {
AssstsTree::AssstsTree(ProjectPtr in_project)
    : wxDataViewModel(),
      p_Root(in_project),
      p_metadata_flctory_ptr_(std::make_shared<MetadataFactory>()),
      slot_childAdd(),
      slot_thisChange(),
      slot_childDelete() {
  slot_childAdd    = [this](const wxDataViewItem& parent,
                         const wxDataViewItem& item) { this->ItemAdded(parent, item); };
  slot_thisChange  = [this](const wxDataViewItem& item) { this->ItemChanged(item); };
  slot_childDelete = [this](const wxDataViewItem& parent,
                            const wxDataViewItem& item) { this->ItemDeleted(parent, item); };
}

unsigned int AssstsTree::GetColumnCount() const {
  return 1;
}

wxString AssstsTree::GetColumnType(unsigned int col) const {
  return "string";
}

void AssstsTree::GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const {
  variant = ConvStr<wxString>("None");
  DOLE_CHECK(item, );
  auto str = reinterpret_cast<Metadata*>(item.GetID());
  variant  = ConvStr<wxString>(str->showStr());
}

bool AssstsTree::SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) {
  return false;
}

wxDataViewItem AssstsTree::GetParent(const wxDataViewItem& item) const {
  DOLE_CHECK(item, wxDataViewItem{});
  const auto k_p_metadata = reinterpret_cast<Metadata*>(item.GetID());
  if (k_p_metadata->hasParent())
    return wxDataViewItem{k_p_metadata->getParent().get()};
  else
    return wxDataViewItem{};
}

bool AssstsTree::IsContainer(const wxDataViewItem& item) const {
  DOLE_CHECK(item, true);

  auto k_item = reinterpret_cast<Metadata*>(item.GetID());
  return k_item->hasChild();
}

unsigned int AssstsTree::GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const {
  //  DOLE_CHECK(item, 0);

  auto k_item = reinterpret_cast<Metadata*>(item.GetID());

  if (!item.IsOk() && p_Root) {
    //这里是空指针的情况， 即没有父级， 我们要使用根来确认
    k_item = p_Root.get();
  } else
    return 0;
  k_item->select_indb(p_metadata_flctory_ptr_);
  k_item->sortChildItems();

  const auto& k_child = k_item->getChildItems();
  for (const auto& k_t : k_child) {
    connectSig(k_t);
    children.Add(wxDataViewItem{k_t.get()});
  }
  return children.size();
}

void AssstsTree::setRoot(const ProjectPtr& in_project) {
  p_Root = in_project;
  Cleared();
}
void AssstsTree::connectSig(const MetadataPtr& in_metadata) const {
  in_metadata->sig_childAdd.connect(
      [this, in_metadata](const MetadataPtr& child) {
        this->slot_childAdd(wxDataViewItem{in_metadata.get()}, wxDataViewItem{child.get()});
      });
  in_metadata->sig_thisChange.connect(
      [this, in_metadata]() {
        this->slot_thisChange(wxDataViewItem{in_metadata.get()});
      });
  in_metadata->sig_childDelete.connect(
      [this, in_metadata](const MetadataPtr& child) {
        this->slot_childDelete(wxDataViewItem{in_metadata.get()}, wxDataViewItem{child.get()});
      });
}
}  // namespace doodle

#undef DOLE_CHECK
