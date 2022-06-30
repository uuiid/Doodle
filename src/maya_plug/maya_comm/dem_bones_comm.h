//
// Created by TD on 2022/6/30.
//

#pragma once

#include <maya_plug/maya_plug_fwd.h>
#include <DemBones/DemBonesExt.h>

namespace doodle::maya_plug {
class dem_bones_ex : public ::Dem::DemBonesExt<std::double_t, std::float_t> {
 public:
  dem_bones_ex()  = default;
  ~dem_bones_ex() = default;
  // 初始化中每次分割骨簇之前调用回调函数。
  void cbInitSplitBegin() override{};
  // 初始化中每次对骨簇进行分割后都会调用回调函数
  void cbInitSplitEnd() override{};
  // 在每次全局迭代更新之前调用回调函数
  void cbIterBegin() override{};
  // 在每次全局迭代更新后调用回调函数，如果返回true，则停止迭代。
  bool cbIterEnd() override { return false; };
  // 在每个外观权重更新之前调用的回调函数
  void cbWeightsBegin() override{};
  // 每次蒙皮权重更新后调用的回调函数
  void cbWeightsEnd() override{};
  // 在每次骨骼转换更新之前调用的回调函数
  void cbTranformationsBegin() override{};
  // 每次骨骼转换更新后调用的回调函数
  void cbTransformationsEnd() override{};

  // 每个局部骨骼转换更新迭代后调用的回调函数，如果返回true，则停止迭代
  void cbWeightsIterBegin() override{};
  //	在每个局部权重更新迭代后调用的回调函数，如果返回true，则停止迭代。
  bool cbWeightsIterEnd() override { return false; };
};

namespace dem_bones_comm_ns {
constexpr char name[] = "doodle_comm_dem_bones";
MSyntax syntax();
}  // namespace dem_bones_comm_ns

class dem_bones_comm : public doodle::TemplateAction<
                           dem_bones_comm,
                           dem_bones_comm_ns::name,
                           dem_bones_comm_ns::syntax> {
  class impl;
  std::unique_ptr<impl> p_i;
  void get_arg(const MArgList& in_arg);

 public:
  dem_bones_comm();
  ~dem_bones_comm() override;
  MStatus doIt(const MArgList& in_arg) override;
};

}  // namespace doodle::maya_plug
