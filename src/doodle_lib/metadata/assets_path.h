//
// Created by TD on 2021/5/18.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/metadata/leaf_meta.h>
#include <doodle_lib/metadata/tree_adapter.h>
namespace doodle {

class DOODLELIB_API assets_path_vector {
  /**
   * @brief 服务器路径
   *
   */
  FSys::path p_server_path;
  /**
   * @brief 服务器中如果存在文件的备份路径
   *
   */
  FSys::path p_backup_path;

  void init();

 public:
  /**
   * @brief 上传时的本地路径
   *
   */
  std::vector<FSys::path> p_local_paths;
  FSys::path p_local_path;

  assets_path_vector();
  DOODLE_MOVE(assets_path_vector);
  /**
   * @brief 按照所在组件的实体之间生成路径
   *
   * @return true
   * @return false
   */
  bool make_path();
  /**
   * @brief 给定实体,生成路径
   *
   * @param in_metadata
   * @return true
   * @return false
   */

  bool make_path(const entt::handle &in_metadata);
  /**
   * @brief 给定实体和本地路径，生成相对路径的服务器路径
   *
   * @param in_metadata
   * @param in_path
   * @return true
   * @return false
   */
  bool make_path(const entt::handle &in_metadata, const FSys::path &in_path);

  [[nodiscard]] const FSys::path &get_local_path() const;
  [[nodiscard]] FSys::path get_cache_path() const;
  [[nodiscard]] const FSys::path &get_server_path() const;
  [[nodiscard]] const FSys::path &get_backup_path() const;

  command_ptr add_file(const FSys::path &in_path);

  std::vector<FSys::path> list();

  [[nodiscard]] rpc_trans_path_ptr_list make_up_path() const;
  [[nodiscard]] rpc_trans_path_ptr_list make_down_path(const FSys::path &in_down_path) const;
  // operator rpc_trans_path_ptr_list() const;

 private:
  //这里是序列化的代码
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 2) {
      ar &BOOST_SERIALIZATION_NVP(p_server_path);
      ar &BOOST_SERIALIZATION_NVP(p_backup_path);
      ar &BOOST_SERIALIZATION_NVP(p_local_paths);
    }
  };

  friend void to_json(nlohmann::json &j, const assets_path_vector &p) {
    j["server_path"] = p.p_server_path;
    j["backup_path"] = p.p_backup_path;
    j["local_paths"] = p.p_local_paths;
  }
  friend void from_json(const nlohmann::json &j, assets_path_vector &p) {
    j.at("server_path").get_to(p.p_server_path);
    j.at("backup_path").get_to(p.p_backup_path);
    j.at("local_paths").get_to(p.p_local_paths);
  }
};

}  // namespace doodle

namespace fmt {

template <>
struct fmt::formatter<doodle::assets_path_vector> : fmt::formatter<fmt::string_view> {
  template <typename FormatContext>
  auto format(const doodle::assets_path_vector &in_, FormatContext &ctx) -> decltype(ctx.out()) {
    std::string str{in_.get_server_path().generic_string()};

    return formatter<string_view>::format(
        str,
        ctx);
  }
};

}  // namespace fmt

BOOST_CLASS_VERSION(doodle::assets_path_vector, 2)
BOOST_CLASS_EXPORT_KEY(doodle::assets_path_vector)
