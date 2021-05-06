//
// Created by teXiao on 2020/10/19.
//

#pragma once
#include <doodle_GUI/doodle_global.h>

#include <loggerlib/Logger.h>

#include <corelib/core/coreset.h>

namespace doodle {
class SettingWidght : public wxFrame {
  wxWindowIDRef p_dep_id;
  wxWindowIDRef p_user_id;
  wxWindowIDRef p_cache_Text_id;
  wxWindowIDRef p_Doc_id;
  wxWindowIDRef p_Maya_id;
  wxWindowIDRef p_Project_id;
  wxWindowIDRef p_Project_delete_id;
  wxWindowIDRef p_Project_add_id;
  wxWindowIDRef p_ue_path_id;
  wxWindowIDRef p_ue_version_id;
  wxWindowIDRef p_ue_shot_start_id;
  wxWindowIDRef p_ue_shot_end_id;
  wxWindowIDRef p_save_id;

  wxComboBox* p_dep;
  wxTextCtrl* p_user;
  wxStaticText* p_cache_Text;
  wxStaticText* p_Doc;
  wxTextCtrl* p_Maya;
  wxComboBox* p_Project;
  wxTextCtrl* p_ue_path;
  wxTextCtrl* p_ue_version;
  wxSpinCtrl* p_ue_shot_start;
  wxSpinCtrl* p_ue_shot_end;

  void InitSetting();
  void InsertProject();
 public:
  explicit SettingWidght(wxWindow* parent, wxWindowID id);

  void AddProject();
};
};  // namespace doodle
