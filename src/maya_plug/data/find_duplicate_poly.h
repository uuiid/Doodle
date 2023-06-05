//
// Created by TD on 2022/3/4.
//

#pragma once

#include <maya_plug/data/maya_poly_info.h>
#include <maya_plug/main/maya_plug_fwd.h>

#include "entt/entity/fwd.hpp"
#include <maya/MObject.h>
#include <vector>

namespace doodle::maya_plug {
/**
 * @brief 寻找maya obj 对
 *
 * 会擦找 maya_poly_info 相等的哦比较 对, 并且第一个是 有皮肤簇节点的, 第二个是有 布料解算节点的obj 对
 */
class find_duplicate_poly {
  std::vector<std::pair<MObject, MObject>> duplicate_objs_{};

 public:
  find_duplicate_poly() = default;
  explicit find_duplicate_poly(const entt::handle& in_handle);
  /**
   * @brief 将 传入的obj列表中寻找符合条件的maya obj mesh 对
   * @param in_array 传入要寻找的范围
   * @return 寻找到的obj对
   */
  std::vector<std::pair<MObject, MObject>> operator()(const MObjectArray& in_array);
  std::vector<std::pair<MObject, MObject>> operator()(const std::vector<MObject>& in_array);
  MObject operator[](const MObject& in_obj);
};

}  // namespace doodle::maya_plug
