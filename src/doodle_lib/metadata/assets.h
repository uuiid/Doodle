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
  /**
   * @brief 初始化时的标签
   *
   * @param in_name
   */
  explicit assets(FSys::path in_name);

  /**
   * @brief 返回字符串表示
   *
   * @return std::string p_path.generic_string()
   */
  [[nodiscard]] std::string str() const;

  /**
   * @brief 获取标签, 使用 std::vector<std::string> 获取标签
   *
   * @return const std::vector<std::string>&
   */
  const std::vector<std::string>& get_path_component() {
    return p_component;
  };

  /**
   * @brief 设置标签路径, 同时会分解路径
   *
   * @param in_path 传入的标签路径
   */
  void set_path(const FSys::path& in_path);
  const FSys::path& get_path() const;
  /**
   * @brief 弃用
   *
   * @return std::string
   */
  [[deprecated("返回始终为空")]] std::string show_str() const;

  /**
   * @brief 排序类, 使用 p_paht 排序
   */
  bool operator<(const assets& in_rhs) const;
  /**
   * @copybrief operator<(const assets& in_rhs) const
   */
  bool operator>(const assets& in_rhs) const;
  /**
   * @copybrief operator<(const assets& in_rhs) const
   */
  bool operator<=(const assets& in_rhs) const;
  /**
   * @copybrief operator<(const assets& in_rhs) const
   */
  bool operator>=(const assets& in_rhs) const;
  /**
   * @brief 相等函数 使用 p_path 判断
   */
  bool operator==(const assets& in_rhs) const;
  /**
   * @copybrief operator==(const assets& in_rhs) const
   */
  bool operator!=(const assets& in_rhs) const;

 private:
  /**
   * @brief 序列化函数
   *
   * @param j json
   * @param p 资产
   */
  friend void to_json(nlohmann::json& j, const assets& p) {
    j["path"] = p.p_path;
  }
  /**
   * @copybrief void to_json(nlohmann::json& j, const assets& p)
   */
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
