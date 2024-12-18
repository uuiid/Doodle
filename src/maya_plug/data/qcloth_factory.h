//
// Created by td_main on 2023/4/27.
//
#pragma once
#include <maya_plug/data/cloth_interface.h>
namespace doodle::maya_plug {

class qcloth_factory : public cloth_factory_interface::element_type {
 public:
  /// 整个场景
  static bool has_cloth();
  std::vector<cloth_interface> create_cloth() const override;
};

}  // namespace doodle::maya_plug
