//
// Created by TD on 2021/5/6.
//

#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/FileWarp/ImageSequence.h>
#include <DoodleLib/FileWarp/Ue4Project.h>
#include <DoodleLib/Metadata/Episodes.h>
#include <DoodleLib/Metadata/MetadataFactory.h>
#include <DoodleLib/Metadata/Model/AssetsTree.h>
#include <DoodleLib/Metadata/Project.h>
#include <DoodleLib/core/CoreSet.h>

#define DOLE_CHECK(item, value) \
  if (!(item).IsOk()) return value;
namespace doodle {
AssetsTree::AssetsTree(ProjectPtr in_project)
    : wxDataViewModel(),
      p_Root(std::move(in_project)),
      p_metadata_flctory_ptr_(std::make_shared<MetadataFactory>()),
      p_metadata_cuttent(),
      slot_childAdd(),
      slot_thisChange(),
      slot_childDelete() {
  slot_childAdd    = [this](const wxDataViewItem& parent,
                         const wxDataViewItem& item) { this->ItemAdded(parent, item); };
  slot_thisChange  = [this](const wxDataViewItem& item) { this->ItemChanged(item); };
  slot_childDelete = [this](const wxDataViewItem& parent,
                            const wxDataViewItem& item) { this->ItemDeleted(parent, item); };
}

unsigned int AssetsTree::GetColumnCount() const {
  return 1;
}

wxString AssetsTree::GetColumnType(unsigned int col) const {
  return "string";
}

void AssetsTree::GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const {
  variant = ConvStr<wxString>("None");
  DOLE_CHECK(item, );
  auto str = reinterpret_cast<Metadata*>(item.GetID());
  variant  = ConvStr<wxString>(str->showStr());
}

bool AssetsTree::SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) {
  return false;
}

wxDataViewItem AssetsTree::GetParent(const wxDataViewItem& item) const {
  DOLE_CHECK(item, wxDataViewItem{});
  const auto k_p_metadata = reinterpret_cast<Metadata*>(item.GetID());
  if (k_p_metadata->hasParent())
    return wxDataViewItem{k_p_metadata->getParent().get()};
  else
    return wxDataViewItem{};
}

bool AssetsTree::IsContainer(const wxDataViewItem& item) const {
  DOLE_CHECK(item, true);

  auto k_item = reinterpret_cast<Metadata*>(item.GetID());
  return k_item->hasChild();
}

unsigned int AssetsTree::GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const {
  //  DOLE_CHECK(item, 0);

  auto k_item = reinterpret_cast<Metadata*>(item.GetID());

  if (!item.IsOk() && p_Root) {
    //这里是空指针的情况， 即没有父级， 我们要使用根来确认
    k_item = p_Root.get();
  } else if (!p_Root) {
    return 0;
  }
  k_item->select_indb(p_metadata_flctory_ptr_);
  k_item->sortChildItems();

  const auto& k_child = k_item->getChildItems();
  for (const auto& k_t : k_child) {
    connectSig(k_t);
    children.Add(wxDataViewItem{k_t.get()});
  }
  return children.size();
}

bool AssetsTree::GetAttr(const wxDataViewItem& in_item, std::uint32_t in_col, wxDataViewItemAttr& attr) const {
  if (!in_item.IsOk())
    return false;

  if (!p_metadata_cuttent)
    return false;

  auto k_item = reinterpret_cast<Metadata*>(in_item.GetID());
  if (*p_metadata_cuttent == *k_item) {
    attr.SetBackgroundColour({200, 75, 49});
    return true;
  } else {
    return false;
  }
}

void AssetsTree::set_current(const MetadataPtr& in_item) {
  auto k_i           = p_metadata_cuttent;
  p_metadata_cuttent = in_item;
  if (k_i) {
    // ItemChanged(wxDataViewItem{k_i.get()});
  }
}

void AssetsTree::setRoot(const ProjectPtr& in_project) {
  p_Root = in_project;
  if (!Cleared())
    throw DoodleError{"无法清除树中的项目"};
}
void AssetsTree::connectSig(const MetadataPtr& in_metadata) const {
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
