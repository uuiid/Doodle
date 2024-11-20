//
// Created by TD on 2021/5/7.
//

#pragma once

#include "doodle_core/metadata/detail/user_ref.h"
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/metadata.h>

namespace doodle {

class assets_file;

/**
 * @brief 这个类代表着服务端的文件条目
 *
 */
class DOODLE_CORE_API assets_file : boost::equality_comparable<assets_file> {
 public:
 private:
  class impl;
  std::unique_ptr<impl> p_i;
  user_ref user_ref{};

  template <typename T>
  friend struct database_n::sql_com;

 public:
  /**
   * @brief 默认构造
   *
   */
  assets_file();
  virtual ~assets_file();
  /**
   * @brief 这是相对于上下文的根目录构建的
   *
   * @param in_path 路径（相对于项目根目录）
   * @param in_name 名称
   * @param in_version 版本
   */
  explicit assets_file(const FSys::path& in_path, std::string in_name, std::uint32_t in_version);
  explicit assets_file(const FSys::path& in_path);

  assets_file(assets_file&&) noexcept;
  assets_file& operator=(assets_file&&) noexcept;

  assets_file(const assets_file&) noexcept;
  assets_file& operator=(const assets_file&) noexcept;

  [[nodiscard]] std::string str() const;
  [[nodiscard]] const std::string& name_attr() const;
  void name_attr(const std::string& in_name) const;

  [[nodiscard]] entt::handle user_attr() const;
  [[nodiscard]] entt::handle user_attr();
  void user_attr(const entt::handle& in_user);

  [[nodiscard]] const FSys::path& path_attr() const;
  void path_attr(const FSys::path& in_path);

  [[nodiscard]] const std::uint32_t& version_attr() const noexcept;
  void version_attr(const std::uint32_t& in_Version) noexcept;

  [[nodiscard]] const std::string& organization_attr() const noexcept;
  void organization_attr(const std::string& in_organization) noexcept;

  [[nodiscard]] const entt::handle& assets_attr() const noexcept;
  void assets_attr(const entt::handle& in_assets_attr) noexcept;

  [[nodiscard]] FSys::path get_path_normal() const;

  bool operator<(const assets_file& in_rhs) const;
  bool operator==(const assets_file& in_rhs) const;

 private:
  friend void DOODLE_CORE_API to_json(nlohmann::json& j, const assets_file& p);
  friend void DOODLE_CORE_API from_json(const nlohmann::json& j, assets_file& p);
};

namespace assets_file_helper {
struct database_t {
  std::int32_t id_{};
  uuid uuid_id_{};
  std::string label_{};
  FSys::path path_{};
  std::string notes_{};
  // 激活
  bool active_{};
  uuid uuid_parent_{};
  std::int32_t parent_id_{};
  bool has_thumbnail_{};
  std::string extension_{};

  friend void DOODLE_CORE_API to_json(nlohmann::json& j, const database_t& p) {
    j["id"]            = p.uuid_id_;
    j["label"]         = p.label_;
    j["path"]          = p.path_;
    j["notes"]         = p.notes_;
    j["active"]        = p.active_;
    j["parent_id"]     = p.uuid_parent_;
    j["has_thumbnail"] = p.has_thumbnail_;
    j["extension"]     = p.extension_;
  }
  friend void DOODLE_CORE_API from_json(const nlohmann::json& j, database_t& p) {
    j.at("label").get_to(p.label_);
    j.at("path").get_to(p.path_);
    j.at("notes").get_to(p.notes_);
    j.at("active").get_to(p.active_);
    j.at("parent_id").get_to(p.uuid_parent_);
    if (j.contains("has_thumbnail")) j.at("has_thumbnail").get_to(p.has_thumbnail_);
    if (j.contains("extension"))
      j.at("extension").get_to(p.extension_);
    else
      p.extension_ = ".png";
  }
};
}  // namespace assets_file_helper
}  // namespace doodle
namespace fmt {
/**
 * @brief 格式化资产文件, 使用name属性
 *
 */
template <>
struct formatter<::doodle::assets_file> : formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const ::doodle::assets_file& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    return formatter<std::string_view>::format(in_.name_attr(), ctx);
  }
};
}  // namespace fmt
