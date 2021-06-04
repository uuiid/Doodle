//
// Created by TD on 2021/5/6.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <wx/dataview.h>
namespace doodle {
class AssetsTree : public wxDataViewModel {
  ProjectPtr p_Root;
  MetadataFactoryPtr p_metadata_flctory_ptr_;

  /**
   *
   * @param in_metadata 输入的要连接的数据
   */
  void connectSig(const MetadataPtr& in_metadata) const;
  std::function<void(const wxDataViewItem& parent, const wxDataViewItem& item)> slot_childAdd;
  std::function<void(const wxDataViewItem& item)> slot_thisChange;
  std::function<void(const wxDataViewItem& parent, const wxDataViewItem& item)> slot_childDelete;

 public:
  explicit AssetsTree(ProjectPtr in_project = {});

  [[nodiscard]] unsigned int GetColumnCount() const override;
  [[nodiscard]] wxString GetColumnType(unsigned int col) const override;
  void GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const override;
  bool SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) override;
  [[nodiscard]] wxDataViewItem GetParent(const wxDataViewItem& item) const override;
  [[nodiscard]] bool IsContainer(const wxDataViewItem& item) const override;
  unsigned int GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const override;

  virtual bool IsListModel() const override { return false; }

  void setRoot(const ProjectPtr& in_project);
};
}  // namespace doodle
