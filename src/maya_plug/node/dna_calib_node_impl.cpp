#include "dna_calib_node_impl.h"

#include <maya/MAngle.h>
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

  switch (dna_calib_dna_reader_->getRotationUnit()) {
    case dna::RotationUnit::degrees:
      to_mangle_func_ = [](std::double_t v) { return MAngle{v, MAngle::kDegrees}; };
      break;
    case dna::RotationUnit::radians:
      to_mangle_func_ = [](std::double_t v) { return MAngle{v, MAngle::kRadians}; };
      break;
    default:
      to_mangle_func_ = [](std::double_t v) { return MAngle(v); };
      break;
  }
  switch (dna_calib_dna_reader_->getTranslationUnit()) {
    case dna::TranslationUnit::cm:
      trans_scale_ = 1.0;
      break;
    case dna::TranslationUnit::m:
      trans_scale_ = 100.0;
      break;
    default:
      trans_scale_ = 1.0;
      break;
  }

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
  rig_cfg.rotationOrder = rl4::RotationOrder::XYZ;  // 目前 vcpkg 构建启用了 RL_BUILD_WITH_XYZ_ROTATION_ORDER

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
  build_joint_output_map();

  return MS::kSuccess;
}
MStatus dna_calib_node::impl_t::compute() {
  if (!rig_logic_ptr_ || !rig_instance_ptr_) return display_error("Rig 数据未初始化"), MS::kFailure;
  if (!dnac::Status::isOk()) {
    return display_error("RigLogic 处于错误状态，拒绝计算: {}", dnac::Status::get().message), MS::kFailure;
  }

  // RigLogic 的计算输入以 raw controls(+PSD) 为核心；若上层只设置了 GUI controls，
  // 需要显式做一次 GUI->Raw 映射，否则 raw controls 保持为 0，会导致 joints 等输出全 0。
  {
    rig_logic_ptr_->mapGUIToRawControls(rig_instance_ptr_.get());
    if (!dnac::Status::isOk()) {
      return display_error("RigLogic::mapGUIToRawControls 失败: {}", dnac::Status::get().message), MS::kFailure;
    }
  }

  rig_logic_ptr_->calculate(rig_instance_ptr_.get());
  if (!dnac::Status::isOk()) {
    return display_error("RigLogic::calculate 失败: {}", dnac::Status::get().message), MS::kFailure;
  }

  // {
  //   rl4::Stats l_stats{};
  //   rig_logic_ptr_->collectCalculationStats(rig_instance_ptr_.get(), &l_stats);
  //   display_info(
  //       "RigLogic stats (after calculate): lod={}, joints={}, jointDeltaValueCount={}, blendShapeChannels={}, "
  //       "animatedMaps={}, psd={}, neuralNets={}, rbfSolvers={} ",
  //       rig_instance_ptr_->getLOD(), l_stats.jointCount, l_stats.jointDeltaValueCount,
  //       l_stats.blendShapeChannelCount, l_stats.animatedMapCount, l_stats.psdCount, l_stats.neuralNetworkCount,
  //       l_stats.rbfSolverCount
  //   );
  // }
  return MS::kSuccess;
}

void dna_calib_node::impl_t::build_joint_output_map() {
  if (!dna_calib_dna_reader_) return;

  const auto l_joint_count = dna_calib_dna_reader_->getJointCount();
  const auto l_row_count   = dna_calib_dna_reader_->getJointRowCount();
  const auto l_lod_count   = dna_calib_dna_reader_->getLODCount();

  // ---- 1. 初始化 ----
  joint_decode_cache_.clear();
  joint_decode_cache_.resize(l_lod_count);
  for (auto& lod_cache : joint_decode_cache_) {
    lod_cache.resize(l_row_count);
  }
  joint_attr_output_map_.clear();
  joint_attr_output_map_.resize(l_lod_count);
  for (auto& lod_map : joint_attr_output_map_) {
    lod_map.resize(l_joint_count);
  }

  // ---- 2. 使用 getJointVariableAttributeIndices 构建映射 ----
  // 遍历所有 LOD，从最高细节 (LOD 0) 到最低，收集参与计算的属性索引。
  // 每个属性索引即 output row，编码方式: joint_index = row / 9, attribute_index = row % 9
  // （项目配置为 EulerAngles，每关节 9 个属性: tx,ty,tz,rx,ry,rz,sx,sy,sz）
  for (std::uint16_t lod = 0; lod < l_lod_count; ++lod) {
    auto l_var_attr_indices = dna_calib_dna_reader_->getJointVariableAttributeIndices(lod);
    for (std::size_t i = 0; i < l_var_attr_indices.size(); ++i) {
      const auto l_out_row   = l_var_attr_indices[i];
      const auto l_joint_idx = static_cast<std::uint16_t>(l_out_row / 9);
      const auto l_attr_idx  = static_cast<std::uint16_t>(l_out_row % 9);

      // 填写 decode cache（output row → joint + attribute）
      if (l_out_row < l_row_count) {
        joint_decode_cache_[lod][l_out_row].joint_index_     = l_joint_idx;
        joint_decode_cache_[lod][l_out_row].attribute_index_ = l_attr_idx;

        // 获取静止位姿数据
        switch (l_attr_idx) {
          case 0:  // tx
            joint_decode_cache_[lod][l_out_row].neutral_value_ =
                dna_calib_dna_reader_->getNeutralJointTranslation(l_joint_idx).x;
            break;
          case 1:  // ty
            joint_decode_cache_[lod][l_out_row].neutral_value_ =
                dna_calib_dna_reader_->getNeutralJointTranslation(l_joint_idx).y;
            break;
          case 2:  // tz
            joint_decode_cache_[lod][l_out_row].neutral_value_ =
                dna_calib_dna_reader_->getNeutralJointTranslation(l_joint_idx).z;
            break;
          case 3:  // rx
          case 4:  // ry
          case 5:  // rz
            // maya骨骼的旋转值必须是 0
            joint_decode_cache_[lod][l_out_row].neutral_value_ = 0;
            break;
          case 6:  // sx
          case 7:  // sy
          case 8:  // sz
            joint_decode_cache_[lod][l_out_row].neutral_value_ = 1;
            break;
        }
      }

      // 填写 per-joint 反向映射（joint → output row[]）
      if (l_joint_idx < l_joint_count && l_attr_idx < 9) {
        joint_attr_output_map_[lod][l_joint_idx].output_indices[l_attr_idx] = static_cast<std::int32_t>(l_out_row);
      } else {
        display_warning(
            "LOD {} 的 output row {} 对应的 joint index {} 或 attribute index {} 超出范围，跳过", lod, l_out_row,
            l_joint_idx, l_attr_idx
        );
      }
    }
  }
}

}  // namespace doodle::maya_plug