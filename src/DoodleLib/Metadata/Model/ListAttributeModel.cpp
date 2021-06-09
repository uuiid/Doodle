//
// Created by TD on 2021/6/7.
//

#include "ListAttributeModel.h"

#include <DoodleLib/Metadata/AssetsFile.h>
#include <DoodleLib/Metadata/Comment.h>
#include <DoodleLib/Metadata/MetadataFactory.h>
#include <date/date.h>

#include <boost/numeric/conversion/cast.hpp>

namespace doodle {
ListAttributeModel::ListAttributeModel()
    : p_metadata(),
      p_metadata_flctory_ptr_(std::make_shared<MetadataFactory>()) {
}

unsigned int ListAttributeModel::GetColumnCount() const {
  return 6;
}

wxString ListAttributeModel::GetColumnType(unsigned int col) const {
  wxString str{};

  switch (col) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    default:
      str = ConvStr("string");
      break;
  }
  return str;
}

void ListAttributeModel::GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const {
  if (!item.IsOk())
    return;

  auto k_ass = reinterpret_cast<AssetsFile*>(item.GetID());
  if (!k_ass)
    return;

  switch (col) {
    case 0:
      variant = ConvStr<wxString>(std::to_string(k_ass->getId()));
      break;
    case 1:
      variant = ConvStr<wxString>(k_ass->getVersionStr());
      break;
    case 2:
      variant = ConvStr<wxString>(k_ass->showStr());
      break;
    case 3: {
      auto& k_com = k_ass->getComment();
      if (k_com.empty())
        variant = ConvStr<wxString>("none");
      else
        variant = ConvStr<wxString>(k_com.back()->getComment());
    } break;

    case 4: {
      auto& time = k_ass->getTime();
      auto str   = date::format("%Y/%m/%d %H:%M", time);
      variant    = ConvStr<wxString>(str);
    } break;
    case 5:
      variant = ConvStr<wxString>(k_ass->getUser());
      break;

    default:
      variant = ConvStr<wxString>("none");
      break;
  }
}

bool ListAttributeModel::SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) {
  bool k_b{false};

  if (!item.IsOk())
    return k_b;
  auto k_ass = reinterpret_cast<AssetsFile*>(item.GetID());
  if (!k_ass)
    return k_b;

  switch (col) {
    case 0:
    case 1:
    case 2:
      k_b = false;
    case 3: {
      auto k_str = variant.GetString();
      if (k_str.empty())
        break;
      k_ass->addComment(std::make_shared<Comment>(ConvStr<std::string>(k_str)));
      k_b = true;
    } break;

    case 4:
    case 5:
    default:
      k_b = false;
      break;
  }
  return k_b;
}

unsigned int ListAttributeModel::GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const {
  if (item.IsOk())
    return 0;
  if (!p_metadata)
    return 0;
  if (!p_metadata->hasChild())
    return 0;

  p_metadata->select_indb(p_metadata_flctory_ptr_);
  p_metadata->sortChildItems();

  auto& k_c = p_metadata->getChildItems();
  for (const auto& k_i : k_c) {
    if (typeid(*k_i) == typeid(AssetsFile))
      children.emplace_back(wxDataViewItem{k_i.get()});
  }
  return children.size();
}

void ListAttributeModel::setRoot(const MetadataPtr& in_metadata_ptr) {
  p_metadata = in_metadata_ptr;
  Cleared();
}

}  // namespace doodle