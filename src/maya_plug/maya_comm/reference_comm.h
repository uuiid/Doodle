//
// Created by TD on 2021/12/13.
//

#pragma once
#include <maya_plug/main/maya_plug_fwd.h>

namespace doodle::maya_plug {
namespace {

constexpr char set_cloth_cache_path_name[] = "set_cloth_cache_path";
}  // namespace

MSyntax set_cloth_cache_path_syntax();

/**
 * @brief 打开并加载文件
 */

class set_cloth_cache_path
    : public TemplateAction<set_cloth_cache_path, set_cloth_cache_path_name, set_cloth_cache_path_syntax> {
 public:
  MStatus doIt(const MArgList&) override;
};
}  // namespace doodle::maya_plug
