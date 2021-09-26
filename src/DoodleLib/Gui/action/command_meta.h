//
// Created by TD on 2021/9/23.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/action/command.h>

#include <boost/hana.hpp>
#include <boost/hana/tuple.hpp>
namespace doodle {
class DOODLELIB_API comm_project_add : public command_meta {
 private:
  string_ptr p_prj_name;
  string_ptr p_prj_name_short;
  string_ptr p_prj_path;
  ProjectPtr p_root;

 public:
  comm_project_add();
  bool render() override;
  bool add_data(const MetadataPtr& in_parent, const MetadataPtr& in) override;
};

class DOODLELIB_API comm_ass_eps : public command_meta {
 private:
  MetadataPtr p_parent;
  EpisodesPtr p_root;

  std::int32_t p_data;
  bool_ptr use_batch;
  std::int32_t p_end;

  void add_eps(std::int32_t in_begin, std::int32_t in_end);

 public:
  comm_ass_eps();
  bool render() override;
  bool add_data(const MetadataPtr& in_parent, const MetadataPtr& in) override;
};

class DOODLELIB_API comm_ass_shot : public command_meta {
 private:
  MetadataPtr p_parent;
  ShotPtr p_root;

  std::uint64_t p_data;
  std::uint32_t p_shot_ab;

 public:
  comm_ass_shot();
  bool render() override;
  bool add_data(const MetadataPtr& in_parent, const MetadataPtr& in) override;
};

class DOODLELIB_API comm_assets : public command_meta {
 private:
  MetadataPtr p_parent;
  AssetsPtr p_root;

  string p_data;

 public:
  comm_assets();
  bool render() override;
  bool add_data(const MetadataPtr& in_parent, const MetadataPtr& in) override;
};

class DOODLELIB_API comm_ass_season : public command_meta {
 private:
  MetadataPtr p_parent;
  season_ptr p_root;

  std::uint64_t p_data;

 public:
  comm_ass_season();
  bool render() override;
  bool add_data(const MetadataPtr& in_parent, const MetadataPtr& in) override;
};

class DOODLELIB_API comm_ass_file : public command_meta {
 private:
  MetadataPtr p_parent;
  AssetsFilePtr p_root;

  chrono::local_time_pos p_time;
  std::vector<doodle::CommentPtr> p_comm;
  bool has_file;

 public:
  comm_ass_file();
  bool render() override;
  bool add_data(const MetadataPtr& in_parent, const MetadataPtr& in) override;
};
class DOODLELIB_API comm_ass : public command_meta {
  boost::hana::tuple<comm_ass_eps,
                     comm_ass_shot,
                     comm_assets,
                     comm_ass_season>      p_val;

 public:
  comm_ass();
  bool render() override;
  bool add_data(const MetadataPtr& in_parent, const MetadataPtr& in) override;
};
}  // namespace doodle
