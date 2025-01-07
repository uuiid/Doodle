//
// Created by TD on 2021/11/30.
//
#pragma once
#include <maya_plug/data/cloth_interface.h>
#include <maya_plug/main/maya_plug_fwd.h>

#include "entt/entity/fwd.hpp"
#include "maya/MApiNamespace.h"
#include "maya/MString.h"
#include <maya/MSelectionList.h>
#include <maya/MTime.h>
#include <vector>
namespace doodle::maya_plug {
class reference_file;
namespace reference_file_ns {
class generate_file_path_base : boost::less_than_comparable<generate_file_path_base> {
 protected:
  std::string extract_reference_name;
  std::string format_reference_name;
  std::string extract_scene_name;
  std::string format_scene_name;
  bool use_add_range{true};
  template <typename T1, typename Char, typename Enable>
  friend struct fmt::formatter;

  virtual FSys::path get_path() const                               = 0;
  virtual FSys::path get_name(const std::string &in_ref_name) const = 0;

  std::string get_extract_scene_name(const std::string &in_name) const;
  std::string get_extract_reference_name(const std::string &in_name) const;

 public:
  generate_file_path_base()          = default;
  virtual ~generate_file_path_base() = default;

  std::optional<std::string> add_external_string;

  std::pair<MTime, MTime> begin_end_time;
  FSys::path operator()(const reference_file &in_ref) const;
  [[nodiscard("")]] bool operator==(const generate_file_path_base &in) const noexcept;
  [[nodiscard("")]] bool operator<(const generate_file_path_base &in) const noexcept;
};

class generate_fbx_file_path : boost::less_than_comparable<generate_fbx_file_path>, public generate_file_path_base {
  friend struct fmt::formatter<generate_fbx_file_path>;

 private:
  std::string camera_suffix{};
  bool is_camera_attr{};

 protected:
  [[nodiscard("")]] FSys::path get_path() const override;
  [[nodiscard("")]] FSys::path get_name(const std::string &in_ref_name) const override;

 public:
  explicit generate_fbx_file_path();

  void is_camera(bool in_is_camera);

  virtual ~generate_fbx_file_path();
};

class generate_abc_file_path : boost::less_than_comparable<generate_abc_file_path>, public generate_fbx_file_path {
 protected:
  [[nodiscard("")]] FSys::path get_path() const override;
  [[nodiscard("")]] FSys::path get_name(const std::string &in_ref_name) const override;
  friend struct fmt::formatter<generate_file_path_base>;
  bool export_fbx{};

 public:
  explicit generate_abc_file_path();
  virtual ~generate_abc_file_path();

  inline void set_fbx_path(bool is_export_fbx = true) { export_fbx = is_export_fbx; };
};

}  // namespace reference_file_ns

/**
 * @brief 一个类似的引用文件使用名称空间作为引用的定位,
 * 而不是ref节点,
 * 这样我们可以在文件中创建出一个类似的引用, 但不是引用,
 * 并且具有一定引用概念的类
 */
class reference_file : public boost::totally_ordered<reference_file> {
 public:
 private:
  std::string get_file_namespace() const;

  MObject file_info_node_;

  MObject get_ref_node() const;

 public:
  reference_file();
  explicit reference_file(const MObject &in_ref_node);

  [[nodiscard]] MSelectionList get_collision_model() const;

  [[nodiscard]] std::string get_namespace() const;

  [[nodiscard]] bool has_node(const MSelectionList &in_list);
  [[nodiscard]] bool has_node(const MObject &in_node) const;
  inline MObject get_file_info_node() const { return file_info_node_; }
  void set_use_sim(bool in_use_sim);
  bool get_use_sim() const;
  /// 加载引用对应的文件
  void load_file();
  /// 确认引用对应的文件
  bool is_loaded() const;

  /**
   * @brief 获取真正的路径名称
   * @return
   */
  [[nodiscard]] FSys::path get_abs_path() const;
  /**
   * @brief 没有加载的引用和资产不存在的文件返回false 我们认为这不是异常, 属于正常情况
   */
  bool replace_sim_assets_file(const std::map<std::string, FSys::path> &in_sim_file_map);
  bool has_sim_assets_file(const std::map<std::string, FSys::path> &in_sim_file_map) const;

  bool replace_file(const FSys::path &in_handle);
  // 重命名名称空间
  void rename_namespace(const std::string &in_name);
  std::optional<MDagPath> get_field_dag() const;

  enum class export_type : std::uint32_t {
    abc = 1,
    fbx = 2,
  };

  /**
   * @brief 从配置文件中查找需要导出组名称对应的 maya 组 (名称空间为引用空间)
   * @return 导出配置文件中对应的组
   */
  std::optional<MDagPath> export_group_attr() const;

  /**
   * @brief 获取这个抽象引用中所有的 obj
   * @return 选中所有的obj
   */
  MSelectionList get_all_object() const;

  std::vector<MDagPath> get_alll_cloth_obj(const std::vector<cloth_interface> &in_cloth) const;

  friend bool operator==(const reference_file &lhs, const reference_file &rhs) {
    return lhs.file_info_node_ == rhs.file_info_node_;
  }
  friend bool operator<(const reference_file &lhs, const reference_file &rhs) {
    return lhs.get_file_namespace() < rhs.get_file_namespace();
  }
};

class reference_file_factory {
 public:
  reference_file_factory()  = default;
  ~reference_file_factory() = default;

  [[nodiscard]] std::vector<reference_file> create_ref() const;
  [[nodiscard]] std::vector<reference_file> create_ref(const MSelectionList &in_list) const;
};

}  // namespace doodle::maya_plug

namespace fmt {
/**
 * @brief
 */
template <>
struct formatter< ::doodle::maya_plug::reference_file_ns::generate_abc_file_path> : formatter<std::string> {
  template <typename FormatContext>
  auto format(const ::doodle::maya_plug::reference_file_ns::generate_abc_file_path &in_, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    return fmt::format_to(
        ctx.out(), "extract_scene_name : {} extract_reference_name : {} use_add_range : {} add_external_string : {}",
        in_.extract_scene_name, in_.extract_reference_name, in_.use_add_range,
        in_.add_external_string ? *in_.add_external_string : std::string{}
    );
  }
};

template <>
struct formatter< ::doodle::maya_plug::reference_file> : formatter<std::string> {
  template <typename FormatContext>
  auto format(const ::doodle::maya_plug::reference_file &in_, FormatContext &ctx) const -> decltype(ctx.out()) {
    return fmt::format_to(
        ctx.out(),"{}", in_.get_namespace()
    );
  }
};


}  // namespace fmt
