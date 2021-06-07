//
// Created by TD on 2021/6/7.
//

#include "ListAttributeModel.h"

namespace doodle {
ListAttributeModel::ListAttributeModel()
    : p_metadata() {
}

unsigned int ListAttributeModel::GetColumnCount() const {
}

wxString ListAttributeModel::GetColumnType(unsigned int col) const {
}

void ListAttributeModel::GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const {
}

bool ListAttributeModel::SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) {
}

wxDataViewItem ListAttributeModel::GetParent(const wxDataViewItem& item) const {
}

bool ListAttributeModel::IsContainer(const wxDataViewItem& item) const {
}

unsigned int ListAttributeModel::GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const {
}

void ListAttributeModel::setRoot(const MetadataPtr& in_metadata_ptr) {
  p_metadata = in_metadata_ptr;
  Cleared();
}

}  // namespace doodle