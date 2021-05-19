//
// Created by TD on 2021/5/6.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <wx/dataview.h>
namespace doodle{
class AssstsTree :public wxDataViewModel{
  
  std::vector<ProjectPtr> p_Root;
  MetadataFactoryPtr p_metadata_flctory_ptr_;

  void connectSig(const MetadataPtr& in_metadata) const;
 public:
  explicit AssstsTree();

  [[nodiscard]] unsigned int GetColumnCount() const override;
  [[nodiscard]] wxString GetColumnType(unsigned int col) const override;
  void GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const override;
  bool SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) override;
  [[nodiscard]] wxDataViewItem GetParent(const wxDataViewItem& item) const override;
  [[nodiscard]] bool IsContainer(const wxDataViewItem& item) const override;
  unsigned int GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const override;
};
}



