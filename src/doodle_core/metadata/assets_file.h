﻿//
// Created by TD on 2021/5/7.
//

#pragma once
#include <doodle_lib/core/core_set.h>
#include <doodle_core/doodle_core_fwd.h>
#include <metadata/metadata.h>

namespace doodle {
/**
 * @brief 文件代表的类型
 *
 */
enum class assets_file_type : std::uint32_t {
  none = 0,

};

/**
 * @brief 这个类代表着服务端的文件条目
 *
 */
class DOODLE_CORE_EXPORT assets_file {
 public:
  FSys::path path;
  std::string p_name;
  std::uint64_t p_version{};

  std::string p_user;
  std::string organization_p;

 private:
 public:
  /**
   * @brief 默认构造
   *
   */
  assets_file();
  /**
   * @brief 这是相对于上下文的根目录构建的
   *
   * @param in_path 路径（相对于项目根目录）
   * @param in_name 名称
   * @param in_version 版本
   */
  explicit assets_file(const FSys::path& in_path,
                       std::string in_name,
                       std::uint64_t in_version);
  explicit assets_file(const FSys::path& in_path);

  DOODLE_MOVE(assets_file);

  [[nodiscard]] std::string str() const;

  [[nodiscard]] const std::string& get_user() const;
  void set_user(const std::string& in_user);

  [[nodiscard]] const std::uint64_t& get_version() const noexcept;
  void set_version(const std::uint64_t& in_Version) noexcept;

  [[nodiscard]] FSys::path get_path_normal() const;

  bool operator<(const assets_file& in_rhs) const;
  bool operator>(const assets_file& in_rhs) const;
  bool operator<=(const assets_file& in_rhs) const;
  bool operator>=(const assets_file& in_rhs) const;

 private:
  friend void to_json(nlohmann::json& j, const assets_file& p) {
    j["name"]           = p.p_name;
    j["user"]           = p.p_user;
    j["organization_p"] = p.organization_p;
    j["version"]        = p.p_version;
    j["path"]           = p.path;
  }
  friend void from_json(const nlohmann::json& j, assets_file& p) {
    j.at("name").get_to(p.p_name);
    j.at("user").get_to(p.p_user);
    j.at("version").get_to(p.p_version);
    if (j.contains("organization_p"))
      j.at("organization_p").get_to(p.organization_p);
    if (j.contains("path"))
      j.at("path").get_to(p.path);
  }
};

}  // namespace doodle
namespace fmt {
/**
 * @brief 格式化资产文件, 使用name属性
 *
 * @tparam
 */
template <>
struct formatter<::doodle::assets_file> : formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const ::doodle::assets_file& in_, FormatContext& ctx) -> decltype(ctx.out()) {
    return formatter<std::string_view>::format(
        in_.p_name,
        ctx);
  }
};
}  // namespace fmt
