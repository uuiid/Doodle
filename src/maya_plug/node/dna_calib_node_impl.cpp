#include "dna_calib_node_impl.h"

#include <maya/MFnDependencyNode.h>
#include <pma/ScopedPtr.h>
#include <riglogic/RigLogic.h>

#if defined(_WIN32)
#include <Windows.h>
#endif
namespace doodle::maya_plug {

MStatus dna_calib_node::impl_t::open_dna_file() {
  MFnDependencyNode l_fn_dep{self_->thisMObject()};
  MStatus l_status{};
  auto l_file_path_plug = l_fn_dep.findPlug(dna_calib_node::dna_file_path, false, &l_status);
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
  MString l_file_path_ms{};
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_file_path_plug.getValue(l_file_path_ms));
  file_path_ = conv::to_s(l_file_path_ms);

  if (file_path_.empty() || !FSys::exists(file_path_))
    return display_error("dna文件不存在: {}", file_path_.generic_string()), MS::kFailure;
  dna_file_stream_ = dnac::makeScoped<dnac::FileStream>(
      file_path_.generic_string().data(), dnac::FileStream::AccessMode::ReadWrite, dnac::FileStream::OpenMode::Binary
  );
  binary_stream_reader_ = dnac::makeScoped<dnac::BinaryStreamReader>(dna_file_stream_.get());
  binary_stream_reader_->read();
  if (!dnac::Status::isOk()) return display_error("读取dna文件失败: {} ", dnac::Status::get().message), MS::kFailure;

  dna_calib_dna_reader_ = dnac::makeScoped<dnac::DNACalibDNAReader>(binary_stream_reader_.get());
  return MS::kSuccess;
}
MStatus dna_calib_node::impl_t::create_rig_data(std::int16_t in_lod) {
  if (!dna_calib_dna_reader_) return display_error("请先打开dna文件"), MS::kFailure;

  rig_logic_ptr_.reset();
  rig_instance_ptr_.reset();

  // 重要：dnacalib 的 vcpkg 构建目前只启用了部分欧拉旋转顺序宏。
  // 若 rotationType==EulerAngles 且 rotationOrder 未启用，会导致 QuaternionJointsEvaluator 的 strategy 为空而崩溃。
  rl4::Configuration rig_cfg{};
  rig_cfg.rotationType  = rl4::RotationType::EulerAngles;
  rig_cfg.rotationOrder = rl4::RotationOrder::ZYX;

  rig_logic_ptr_        = dnac::makeScoped<rl4::RigLogic>(dna_calib_dna_reader_.get(), rig_cfg, nullptr);
  if (!rig_logic_ptr_ || !dnac::Status::isOk()) {
    const char* msg = dnac::Status::isOk() ? "unknown" : dnac::Status::get().message;
    return display_error("创建 RigLogic 失败: {}", msg), MS::kFailure;
  }

  rig_instance_ptr_ = dnac::makeScoped<rl4::RigInstance>(rig_logic_ptr_.get());
  if (!rig_instance_ptr_ || !dnac::Status::isOk()) {
    const char* msg = dnac::Status::isOk() ? "unknown" : dnac::Status::get().message;
    return display_error("创建 RigInstance 失败: {}", msg), MS::kFailure;
  }

  rig_instance_ptr_->setLOD(in_lod);
  if (!dnac::Status::isOk()) {
    const char* msg = dnac::Status::get().message;
    return display_error("RigInstance::setLOD({}) 失败: {}", in_lod, msg), MS::kFailure;
  }

  return MS::kSuccess;
}
MStatus dna_calib_node::impl_t::compute() {
  if (!rig_logic_ptr_ || !rig_instance_ptr_) return display_error("Rig 数据未初始化"), MS::kFailure;
  if (!dnac::Status::isOk()) {
    return display_error("RigLogic 处于错误状态，拒绝计算: {}", dnac::Status::get().message), MS::kFailure;
  }

  rig_logic_ptr_->calculate(rig_instance_ptr_.get());
  if (!dnac::Status::isOk()) {
    return display_error("RigLogic::calculate 失败: {}", dnac::Status::get().message), MS::kFailure;
  }
  return MS::kSuccess;
}

void dna_calib_node::impl_t::build_joint_output_map() {}

}  // namespace doodle::maya_plug