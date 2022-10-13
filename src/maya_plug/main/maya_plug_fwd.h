//
// Created by TD on 2021/11/22.
//

#pragma once

#include <maya/MGlobal.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MSyntax.h>
#include <maya/MPxCommand.h>

#include <maya_plug/data/maya_tool.h>
#include <maya_plug/exception/exception.h>
#include <maya_plug/fmt/fmt_warp.h>

#include <utility>
namespace doodle {

inline MSyntax null_syntax_t() { return {}; };

using CreateSyntaxFunction = std::add_pointer_t<MSyntax()>;
/**
 * @brief maya 命令模板
 *
 * @tparam ActionClass 动作类
 * @tparam CommandName 命令名称
 * @tparam CommandSyntax 命令标志
 */
template <class ActionClass, const char* CommandName, CreateSyntaxFunction CommandSyntax = null_syntax_t>
class TemplateAction : public MPxCommand {
 public:
  TemplateAction() = default;

  static void* creator() {
    return new ActionClass;
  }
  template <class FNPLUG>
  static MStatus registerCommand(FNPLUG& obj) {
    return obj.registerCommand(CommandName, creator, CommandSyntax);
  }
  template <class FNPLUG>
  static MStatus deregisterCommand(FNPLUG& obj) {
    return obj.deregisterCommand(CommandName);
  }
  [[nodiscard]] bool hasSyntax() const override {
    return true;
  };
};

}  // namespace doodle

/**
 * @brief
 *
 * @todo 在工具架上添加自动设置缓存的工具
 * @todo 添加maya 布料制作工具中的 解锁全局节点
 * @todo 查看doc需求文档
 *
 * @todo 检查引用是否加载, 不加载就不显示
 * @todo 引用清除会出现问题, 要查
 *
 */
namespace doodle::maya_plug {
class play_blast;

/**
 * @brief  字符串转换类
 * 作为maya字符串和标准库字符串之间的转换
 */
class d_str {
 public:
  std::string p_u8_str{};
  template <class MStr, std::enable_if_t<std::is_same_v<MString, MStr>, bool> = true>
  explicit d_str(const MStr& in)
      : p_u8_str(in.asUTF8()){};

  //  explicit d_str(const MString& in)
  //      : p_u8_str(in.asUTF8()){};

  explicit d_str(std::string in_u8_str)
      : p_u8_str(std::move(in_u8_str)) {
  }

  inline operator MString() const {
    MString k_r{};
    k_r.setUTF8(p_u8_str.c_str());
    return k_r;
  }
  inline operator std::string() const {
    return p_u8_str;
  }
  [[nodiscard]] inline std::string str() const {
    return p_u8_str;
  }
};

/**
 * @brief 检查maya的返回状态
 */
#define DOODLE_MAYA_CHICK(in_status)      \
  if (!in_status)                         \
  throw_exception(maya_error{             \
      make_error(in_status.statusCode()), \
      fmt::to_string(in_status)})

void open_windows();

}  // namespace doodle::maya_plug

namespace fmt {
/**
 * @brief 格式化和maya异常
 *
 * @tparam
 */
template <>
struct formatter<::doodle::maya_plug::maya_error> : formatter<string_view> {
  template <typename FormatContext>
  auto format(const ::doodle::maya_plug::maya_error& in_, FormatContext& ctx) -> decltype(ctx.out()) {
    return formatter<string_view>::format(
        boost::diagnostic_information(in_),
        ctx
    );
  }
};

}  // namespace fmt
