//
// Created by TD on 2021/6/7.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <wx/dataview.h>

namespace doodle {
class ListAttributeModel : public wxDataViewModel {
  MetadataPtr p_metadata;

 public:
  explicit ListAttributeModel();

  [[nodiscard]] unsigned int GetColumnCount() const override;
  [[nodiscard]] wxString GetColumnType(unsigned int col) const override;
  void GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const override;
  bool SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) override;
  [[nodiscard]] wxDataViewItem GetParent(const wxDataViewItem& item) const override {
    return {};
  };
  [[nodiscard]] bool IsContainer(const wxDataViewItem& item) const override {
    return !item.IsOk();
  };
  unsigned int GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const override;

  virtual bool IsListModel() const override { return true; }

  void setRoot(const MetadataPtr& in_metadata_ptr);
};

}  // namespace doodle
