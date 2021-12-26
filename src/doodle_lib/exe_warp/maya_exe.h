//
// Created by TD on 2021/12/25.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::details {
class DOODLELIB_API qcloth_arg {
 public:
  FSys::path sim_path;
  FSys::path qcloth_assets_path;
  FSys::path export_path;
  uuid uuid_p;
  bool only_sim;

  friend void to_json(nlohmann::json &nlohmann_json_j, const qcloth_arg &nlohmann_json_t) {
    nlohmann_json_j["path"]               = nlohmann_json_t.sim_path;
    nlohmann_json_j["export_path"]        = nlohmann_json_t.export_path;
    nlohmann_json_j["qcloth_assets_path"] = nlohmann_json_t.qcloth_assets_path;
    nlohmann_json_j["only_sim"]           = nlohmann_json_t.only_sim;
    nlohmann_json_j["uuid"]               = boost::lexical_cast<string>(nlohmann_json_t.uuid_p);
  };
};

class DOODLELIB_API export_fbx_arg {
 public:
  /**
   * @brief maya文件源路径(文件路径)
   *
   */
  FSys::path file_path;
  /**
   * @brief 导出文件的路径(目录)
   *
   */
  FSys::path export_path;
  /**
   * @brief 是否导出所有引用
   *
   */
  bool use_all_ref;

  uuid uuid_p;
  friend void to_json(nlohmann::json &nlohmann_json_j, const export_fbx_arg &nlohmann_json_t) {
    nlohmann_json_j["path"]        = nlohmann_json_t.file_path;
    nlohmann_json_j["export_path"] = nlohmann_json_t.export_path;
    nlohmann_json_j["use_all_ref"] = nlohmann_json_t.use_all_ref;
    nlohmann_json_j["uuid"]        = boost::lexical_cast<string>(nlohmann_json_t.uuid_p);
  };
};

class DOODLELIB_API maya_exe : public process_t<maya_exe> {
  class impl;
  std::unique_ptr<impl> p_i;
  void add_maya_fun_tool() const;

 public:
  using base_type = process_t<maya_exe>;

  //  void succeed() noexcept;
  //  void fail() noexcept;
  //  void pause() noexcept;
  explicit maya_exe(const entt::handle &in_handle, const string &in_comm);
  explicit maya_exe(const entt::handle &in_handle, const qcloth_arg &in_comm);
  explicit maya_exe(const entt::handle &in_handle, const export_fbx_arg &in_comm);
  ~maya_exe() override;

  void init();
  void succeeded();
  void failed();
  void aborted();
  void update(base_type::delta_type, void *data);
};
}  // namespace doodle::details
