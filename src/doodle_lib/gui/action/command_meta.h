//
// Created by TD on 2021/9/23.
//

#pragma once

#include <doodle_lib/Gui/action/command.h>
#include <doodle_lib/Gui/action/command_ue4.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/hana.hpp>
#include <boost/hana/tuple.hpp>
namespace doodle {

class DOODLELIB_API comm_project_add : public command_base {
 private:
  string_ptr p_prj_name;
  string_ptr p_prj_name_short;
  string_ptr p_prj_path;
  project_ptr p_root;

 protected:
  virtual bool set_child(const project_ptr& in_ptr) override;

 public:
  comm_project_add();
  bool render() override;
};

class DOODLELIB_API comm_ass_eps : public command_base {
 private:
  episodes_ptr p_root;

  std::int32_t p_data;
  std::int32_t p_end;
  bool_ptr use_batch;

  void add_eps(const std::vector<std::int32_t>& p_eps);

 protected:
  virtual bool set_child(const episodes_ptr& in_ptr) override;

 public:
  comm_ass_eps();
  bool render() override;
};

class DOODLELIB_API comm_ass_shot : public command_base {
 private:
  shot_ptr p_root;

  std::int32_t p_data;
  std::int32_t p_end;
  bool_ptr use_batch;

  std::string_view p_shot_ab;

  void add_shot(const std::vector<std::int32_t>& p_shots);

 protected:
  virtual bool set_child(const shot_ptr& in_ptr) override;

 public:
  comm_ass_shot();
  bool render() override;
};

class DOODLELIB_API comm_assets : public command_base {
 private:
  assets_ptr p_root;

  string p_data;
  void add_ass(std::vector<string> in_Str);

 protected:
  virtual bool set_child(const assets_ptr& in_ptr) override;

 public:
  comm_assets();
  bool render() override;
};

class DOODLELIB_API comm_ass_season : public command_base {
 private:
  season_ptr p_root;

  std::int32_t p_data;
  std::int32_t p_end;
  bool_ptr use_batch;
  void add_season(const std::vector<std::int32_t>& in);

 protected:
  virtual bool set_child(const season_ptr& in_ptr) override;

 public:
  comm_ass_season();
  bool render() override;
};

class DOODLELIB_API comm_ass_file : public command_base {
 private:
  assets_file_ptr p_root;

  chrono::local_time_pos p_time;
  comment_vector_ptr p_comm;
  bool has_file;

  time_widget_ptr p_time_widget;
  string_ptr p_comm_str;

 protected:
  virtual bool set_child(const assets_file_ptr& in_ptr) override;

 public:
  comm_ass_file();
  bool render() override;
};
class DOODLELIB_API comm_ass : public command_base {
  boost::hana::tuple<comm_ass_season,
                     comm_ass_eps,
                     comm_ass_shot,
                     comm_assets,
                     comm_ass_ue4_create_shot>
      p_val;

 protected:
  virtual bool set_child(const episodes_ptr& in_ptr) override;
  virtual bool set_child(const shot_ptr& in_ptr) override;
  virtual bool set_child(const season_ptr& in_ptr) override;
  virtual bool set_child(const assets_ptr& in_ptr) override;
  virtual bool set_child(const assets_file_ptr& in_ptr) override;
  virtual bool set_child(const project_ptr& in_ptr) override;
  virtual bool set_child(nullptr_t const& in_ptr) override;

 public:
  comm_ass();
  bool render() override;
};
}  // namespace doodle
