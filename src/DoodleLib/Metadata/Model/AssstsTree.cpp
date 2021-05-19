//
// Created by TD on 2021/5/6.
//

#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/FileWarp/ImageSequence.h>
#include <DoodleLib/FileWarp/Ue4Project.h>
#include <DoodleLib/Metadata/Episodes.h>
#include <DoodleLib/Metadata/Model/AssstsTree.h>
#include <DoodleLib/Metadata/Project.h>
#include <DoodleLib/core/coreset.h>
#include <DoodleLib/Metadata/MetadataFactory.h>

#define DOLE_CHECK(item, value) \
  if (!(item).IsOk()) return value;
namespace doodle {
AssstsTree::AssstsTree()
    : wxDataViewModel(),
      p_Root(MetadataSet::Get().getAllProjects()),
      p_metadata_flctory_ptr_(std::make_shared<MetadataFactory>()){

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
  if (k_p_metadata->hasChild())
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
  if (!item.IsOk()) {
    //这里是空指针的情况， 即没有父级， 我们要使用根来确认
    for (const auto& k_t : p_Root) {
      if(k_t == coreSet::getSet().GetMetadataSet().Project_())
        k_t->load(p_metadata_flctory_ptr_);
      children.Add(wxDataViewItem{k_t.get()});
    }
  } else {
    k_item->load(p_metadata_flctory_ptr_);
    k_item->sortChildItems();
    const auto& k_child = k_item->getChildItems();
    for (const auto& k_t : k_child) {
      children.Add(wxDataViewItem{k_t.get()});
    }
  }
  return children.size();
}
void AssstsTree::connectSig(const MetadataPtr& in_metadata) const {
  in_metadata->sig_childAdd.connect(
      [](const MetadataPtr& this_, const MetadataPtr& child){

      });
}
}  // namespace doodle

#undef DOLE_CHECK
