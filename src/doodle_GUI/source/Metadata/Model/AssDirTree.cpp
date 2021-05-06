//
// Created by TD on 2021/5/6.
//

#include <doodle_GUI/source/Metadata/Model/AssDirTree.h>

namespace doodle{
AssDirTree::AssDirTree() {
}
unsigned int AssDirTree::GetColumnCount() const {
  return 1;
}
wxString AssDirTree::GetColumnType(unsigned int col) const {
  return wxString();
}
void AssDirTree::GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const {
}
bool AssDirTree::SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) {
  return false;
}
wxDataViewItem AssDirTree::GetParent(const wxDataViewItem& item) const {
  return wxDataViewItem();
}
bool AssDirTree::IsContainer(const wxDataViewItem& item) const {
  return false;
}
unsigned int AssDirTree::GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const {
  return 0;
}

}
