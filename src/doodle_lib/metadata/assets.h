//
// Created by teXiao on 2021/4/27.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
/**
 * @brief 资产类, 这个时候资产分类
 *
 * 使用path表示资产的分类, 其中每段路径代表一个标签, 标签越靠前权重越高
 *
 */
class DOODLELIB_API assets {
  /**
   * @brief 分解路径,转换为向量缓存
   *
   */
  void set_path_component();

  std::vector<string> p_component;

 public:
  /**
   * @brief 文件标签
   *
   */
  FSys::path p_path;
  /**
   * @brief 使用函子设置文件标签
   *
   */
  class DOODLELIB_API set_path_fun {
   public:
    std::vector<std::string>& p_comm;
    explicit set_path_fun(std::vector<std::string>& in_comm)
        : p_comm(in_comm){};
    void operator()(assets& in) const;
  };

  assets();
  explicit assets(FSys::path in_name);
  // ~Assets();

  [[nodiscard]] std::string str() const;

  const std::vector<std::string>& get_path_component() {
    return p_component;
  };

  void set_path(const FSys::path& in_path);
  const FSys::path& get_path() const;

  [[deprecated("返回始终为空")]] std::string show_str() const;

  bool operator<(const assets& in_rhs) const;
  bool operator>(const assets& in_rhs) const;
  bool operator<=(const assets& in_rhs) const;
  bool operator>=(const assets& in_rhs) const;
  bool operator==(const assets& in_rhs) const;
  bool operator!=(const assets& in_rhs) const;

 private:
  friend void to_json(nlohmann::json& j, const assets& p) {
    j["path"] = p.p_path;
  }
  friend void from_json(const nlohmann::json& j, assets& p) {
    j.at("path").get_to(p.p_path);
    p.set_path_component();
  }
};
}  // namespace doodle
namespace fmt {
/**
 * @brief 格式化资产类
 * 这个使用资产的路径属性格式化
 * @tparam
 */
template <>
struct formatter<::doodle::assets> : formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const ::doodle::assets& in_, FormatContext& ctx) -> decltype(ctx.out()) {
    return formatter<std::string_view>::format(
        in_.p_path.generic_string(),
        ctx);
  }
};
}  // namespace fmt
