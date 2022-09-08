//
// Created by TD on 2021/5/7.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/metadata.h>

namespace doodle {
/**
 * @brief 文件代表的类型
 *
 */
enum class assets_file_type : std::uint32_t {
  none = 0,

};

class assets_file;
void to_json(nlohmann::json& j, const assets_file& p);
void from_json(const nlohmann::json& j, assets_file& p);

/**
 * @brief 这个类代表着服务端的文件条目
 *
 */
class DOODLE_CORE_API assets_file : boost::equality_comparable<assets_file> {
 public:
 private:
  class impl;
  std::unique_ptr<impl> p_i;

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
  explicit assets_file(const FSys::path& in_path, std::string in_name, std::uint64_t in_version);
  explicit assets_file(const FSys::path& in_path);

  assets_file(assets_file&&) noexcept;
  assets_file& operator=(assets_file&&) noexcept;

  assets_file(const assets_file&) noexcept;
  assets_file& operator=(const assets_file&) noexcept;

  [[nodiscard]] std::string str() const;
  [[nodiscard]] const std::string& name_attr() const;
  void name_attr(const std::string& in_name) const;

  [[nodiscard]] entt::handle user_attr() const;
  void user_attr(const entt::handle& in_user);

  [[nodiscard]] const FSys::path& path_attr() const;
  void path_attr(const FSys::path& in_path);

  [[nodiscard]] const std::uint64_t& version_attr() const noexcept;
  void version_attr(const std::uint64_t& in_Version) noexcept;

  [[nodiscard]] const std::string& organization_attr() const noexcept;
  void organization_attr(const std::string& in_organization) noexcept;

  [[nodiscard]] FSys::path get_path_normal() const;

  bool operator<(const assets_file& in_rhs) const;
  bool operator==(const assets_file& in_rhs) const;

 private:
  friend void to_json(nlohmann::json& j, const assets_file& p);
  friend void from_json(const nlohmann::json& j, assets_file& p);
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
  auto format(const ::doodle::assets_file& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    return formatter<std::string_view>::format(
        in_.name_attr(),
        ctx
    );
  }
};
}  // namespace fmt
