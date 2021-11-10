//
// Created by TD on 2021/9/23.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/action/command.h>
#include <doodle_lib/gui/action/command_files.h>
#include <doodle_lib/gui/action/command_ue4.h>

#include <boost/hana.hpp>
#include <boost/hana/tuple.hpp>
namespace doodle {

class DOODLELIB_API comm_project_add : public command_base {
 private:
  string_ptr p_prj_name;
  string_ptr p_prj_name_short;
  string_ptr p_prj_path;
  entt::handle p_root;

 protected:
 public:
  comm_project_add();
  bool render() override;
  virtual bool set_data(const entt::handle& in_data) override;
};

class DOODLELIB_API comm_ass_eps : public command_base {
 private:
  entt::handle p_root;

  std::int32_t p_data;
  std::int32_t p_end;


 protected:
 public:
  comm_ass_eps();
  bool render() override;
  virtual bool set_data(const entt::handle& in_data) override;
};

class DOODLELIB_API comm_ass_shot : public command_base {
 private:
  entt::handle p_root;

  std::int32_t p_data;
  std::int32_t p_end;

  std::string_view p_shot_ab;


 protected:
 public:
  comm_ass_shot();
  bool render() override;
  virtual bool set_data(const entt::handle& in_data) override;
};

class DOODLELIB_API comm_assets : public command_base {
 private:
  entt::handle p_root;

  string p_data;

 protected:
 public:
  comm_assets();
  bool render() override;
  virtual bool set_data(const entt::handle& in_data) override;
};

class DOODLELIB_API comm_ass_season : public command_base {
 private:
  entt::handle p_root;

  std::int32_t p_data;
  std::int32_t p_end;

 protected:
 public:
  comm_ass_season();
  bool render() override;
  virtual bool set_data(const entt::handle& in_data) override;
};

class DOODLELIB_API comm_ass_file_attr : public command_base {
 private:
  entt::handle p_root;

  chrono::local_time_pos p_time;
  bool has_file;

  time_widget_ptr p_time_widget;
  string_ptr p_comm_str;

 protected:
 public:
  comm_ass_file_attr();
  bool render() override;
  virtual bool set_data(const entt::handle& in_data) override;
};

// template <class... in_comm>
// class DOODLELIB_API comm_compound : public command_base {
//   boost::hana::tuple<in_comm...> p_val;

//  public:
//   comm_compound() : command_base(), p_val(){};
//   bool render() override {
//     boost::hana::for_each(p_val, [](auto& in) {
//       dear::TreeNode{in.class_name().c_str()} && [&]() {
//         in.render();
//       };
//     });
//     return true;
//   };
// };

// using comm_ass = comm_compound<
//     comm_ass_season,
//     comm_ass_eps,
//     comm_ass_shot,
//     comm_assets,
//     comm_ass_ue4_create_shot>;

// using comm_assets_file = comm_compound<
//     comm_ass_file_attr,
//     comm_files_select>;

}  // namespace doodle
