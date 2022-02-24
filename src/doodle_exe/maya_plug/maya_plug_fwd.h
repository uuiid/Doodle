//
// Created by TD on 2021/11/22.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <maya/MGlobal.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MSyntax.h>
#include <maya/MPxCommand.h>

#include <maya_plug/data/maya_tool.h>
#include <maya_plug/exception/exception.h>
#include <maya_plug/fmt/fmt_warp.h>
namespace doodle {

inline MSyntax null_syntax_t() { return {}; };

using CreateSyntaxFunction = std::add_pointer_t<MSyntax()>;
template <class ActionClass, const char* CommandName, CreateSyntaxFunction CommandSyntax = null_syntax_t>
class TemplateAction : public MPxCommand {
 public:
  TemplateAction() = default;

  virtual MStatus doIt(const MArgList&) override {
    return MStatus::kFailure;
  }
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

namespace doodle::maya_plug {
class play_blast;
using play_blast_ptr = std::shared_ptr<play_blast>;

class d_str {
 public:
  string p_u8_str{};
  template <class MStr, std::enable_if_t<std::is_same_v<MString, MStr>, bool> = true>
  explicit d_str(const MStr& in)
      : p_u8_str(in.asUTF8()){};

  //  explicit d_str(const MString& in)
  //      : p_u8_str(in.asUTF8()){};

  d_str(const string& in_u8_str)
      : p_u8_str(in_u8_str) {
  }

  inline operator MString() const {
    MString k_r{};
    k_r.setUTF8(p_u8_str.c_str());
    return k_r;
  }
  inline operator string() const {
    return p_u8_str;
  }
  inline string str() const {
    return p_u8_str;
  }
};

// inline void chick_maya_status(const MStatus& in_status) {
//   if (in_status != MStatus::MStatusCode::kSuccess) {
//     const MString& l_string = in_status.errorString();
//     MGlobal::displayError(l_string);
//     DOODLE_LOG_ERROR(l_string.asUTF8());
//     throw doodle_error{l_string.asUTF8()};
//   }
// };

#define DOODLE_CHICK(in_status) \
  throw_maya_exception(in_status, DOODLE_SOURCE_LOC);
}  // namespace doodle::maya_plug

namespace fmt {
/**
 * @brief 格式化和maya异常
 * 
 * @tparam  
 */
template <>
struct fmt::formatter<::doodle::maya_plug::maya_error> : fmt::formatter<fmt::string_view> {
  template <typename FormatContext>
  auto format(const ::doodle::maya_plug::maya_error& in_, FormatContext& ctx) -> decltype(ctx.out()) {
    return formatter<string_view>::format(
        in_.what(),
        ctx);
  }
};

}  // namespace fmt
