//
// Created by TD on 2021/5/6.
//

#pragma once

#include <doodle_GUI/doodle_global.h>
#include <wx/dataview.h>
namespace doodle{



class AssDirTree :public wxDataViewModel{
  std::vector<ProjectPtr> p_Root;
 public:
  explicit AssDirTree();

  [[nodiscard]] unsigned int GetColumnCount() const override;
  [[nodiscard]] wxString GetColumnType(unsigned int col) const override;
  void GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const override;
  bool SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) override;
  [[nodiscard]] wxDataViewItem GetParent(const wxDataViewItem& item) const override;
  [[nodiscard]] bool IsContainer(const wxDataViewItem& item) const override;
  unsigned int GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const override;
};
}



