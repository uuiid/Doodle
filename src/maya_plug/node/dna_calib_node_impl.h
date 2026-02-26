#include <maya_plug/maya_plug_fwd.h>
#include <maya_plug/node/dna_calib_node.h>

#include <array>
#include <dnacalib/dna/DNACalibDNAReader.h>
#include <functional>
#include <maya/MAngle.h>
#include <maya/MPxNode.h>
#include <maya/MStatus.h>
#include <riglogic/RigLogic.h>
#include <riglogic/riglogic/RigInstance.h>
#include <vector>

namespace doodle::maya_plug {
class dna_calib_node::impl_t {
 public:
  impl_t()  = default;
  ~impl_t() = default;

  FSys::path file_path_;
  dna_calib_node* self_;

  dnac::ScopedPtr<dnac::FileStream> dna_file_stream_;
  dnac::ScopedPtr<dnac::BinaryStreamReader> binary_stream_reader_;
  dnac::ScopedPtr<dnac::DNACalibDNAReader> dna_calib_dna_reader_;

  dnac::ScopedPtr<rl4::RigLogic> rig_logic_ptr_;
  dnac::ScopedPtr<rl4::RigInstance> rig_instance_ptr_;

  std::vector<std::uint16_t> arrt_to_gui_control_;

  std::double_t trans_scale_{1.0};
  std::function<MAngle(std::double_t)> to_mangle_func_;

  MStatus open_dna_file();
  MStatus create_rig_data(std::int16_t in_lod = 0);
  // 构建输出joint decode 输出表
  void build_joint_output_map();
  // 执行计算
  MStatus compute();

  // ----------- 骨骼 Decode Cache 类型 -----------
  struct JointDecodeEntry {
    std::uint16_t joint_index_;
    // 0-8 分别对应 tx, ty, tz, rx, ry, rz, sx, sy, sz
    std::uint16_t attribute_index_;
    // 对应的静止位姿数据, 只有属性对应的单独分量, 位置, 旋转 (度, 3v), 缩放分量
    std::double_t neutral_value_;
  };

  // 每个关节的 9 个属性在 getJointOutputs() 中的行索引
  struct JointAttrOutputMap {
    // [0]=tx, [1]=ty, [2]=tz, [3]=rx, [4]=ry, [5]=rz, [6]=sx, [7]=sy, [8]=sz
    // -1 表示该属性在当前 DNA 中不存在
    std::array<std::int32_t, 9> output_indices{-1, -1, -1, -1, -1, -1, -1, -1, -1};
  };

  // 通过 output_row_index 快速查找 joint_index + attribute_index
  // 大小 = getJointRowCount()，索引 = 输出数组下标
  const std::vector<JointDecodeEntry>& joint_decode_cache(std::int16_t in_lod) const {
    return joint_decode_cache_[in_lod];
  }

  // 通过 joint_index 快速查找其 9 个属性在输出数组中的行号
  // 大小 = getJointCount()，索引 = joint_index
  const std::vector<JointAttrOutputMap>& joint_attr_output_map(std::int16_t in_lod) const {
    return joint_attr_output_map_[in_lod];
  }

 private:
  std::vector<std::vector<JointDecodeEntry>> joint_decode_cache_;
  std::vector<std::vector<JointAttrOutputMap>> joint_attr_output_map_;
};
}  // namespace doodle::maya_plug