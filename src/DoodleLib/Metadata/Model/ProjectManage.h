//
// Created by TD on 2021/6/2.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <wx/dataview.h>

namespace doodle {
class DOODLELIB_API ProjectManage : public wxDataViewModel {
  std::vector<ProjectPtr> p_project;
  std::vector<ProjectPtr> p_project_new;
  std::vector<ProjectPtr> p_project_remove;

 public:
  ProjectManage();

  [[nodiscard]] unsigned int GetColumnCount() const override;
  [[nodiscard]] wxString GetColumnType(unsigned int col) const override;
  void GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const override;
  bool SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) override;
  bool GetAttr(const wxDataViewItem& in_item, std::uint32_t in_col, wxDataViewItemAttr& attr) const override;

  [[nodiscard]] wxDataViewItem GetParent(const wxDataViewItem& item) const override {
    return {};
  };

  [[nodiscard]] bool IsContainer(const wxDataViewItem& item) const override {
    return !item.IsOk();
  };
  unsigned int GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const override;

  virtual bool IsListModel() const override { return true; }

  void addProject(const ProjectPtr& prj);
  bool removeProject(const ProjectPtr& prj);

  void submit(const MetadataFactoryPtr& in_factory);
};
}  // namespace doodle
