//
// Created by TD on 2021/10/14.
//

#pragma once


namespace doodle::maya_plug {
class reference_file;

namespace reference_attr {
class data {
 public:
  string path;
  bool use_sim;
  bool operator==(const data& in_rhs) const;
  bool operator!=(const data& in_rhs) const;
  friend void to_json(nlohmann::json& j, const data& p) {
    j["path"]    = p.path;
    j["use_sim"] = p.use_sim;
  }
  friend void from_json(const nlohmann::json& j, data& p) {
    p.path    = j.at("path");
    p.use_sim = j.at("use_sim");
  }
};
using data_ptr = std::shared_ptr<data>;
}  // namespace reference_attr

/**
 * @brief 引用文件标签命令
 *
 * @li 添加引用文件解算标签
 * @li 在没有引用文件标记时， 使用所有载入的应用进行解算
 *
 */
class reference_attr_setting : public process_t<reference_attr_setting> {
  std::vector<entt::handle> p_handle;

  bool get_file_info();
  void clear();

 public:
  reference_attr_setting();
  constexpr static std::string_view name{"引用编辑"};

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(delta_type, void* data);

  bool render();
};

}  // namespace doodle::maya_plug
