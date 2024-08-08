//
// Created by TD on 2021/11/22.
//

#pragma once

#include <doodle_core/doodle_core.h>

#include <maya_plug/data/maya_conv_str.h>
#include <maya_plug/data/maya_tool.h>
#include <maya_plug/exception/exception.h>
#include <maya_plug/fmt/fmt_warp.h>

#include <maya/MGlobal.h>
#include <maya/MPxCommand.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MSyntax.h>
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

  static void* creator() { return new ActionClass; }
  template <class FNPLUG>
  static MStatus registerCommand(FNPLUG& obj) {
    return obj.registerCommand(CommandName, creator, CommandSyntax);
  }
  template <class FNPLUG>
  static MStatus deregisterCommand(FNPLUG& obj) {
    return obj.deregisterCommand(CommandName);
  }
  [[nodiscard]] bool hasSyntax() const override { return true; };
};

}  // namespace doodle

