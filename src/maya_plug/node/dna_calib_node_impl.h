#include <maya_plug/maya_plug_fwd.h>
#include <maya_plug/node/dna_calib_node.h>

#include <dnacalib/dna/DNACalibDNAReader.h>
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

  MStatus open_dna_file();
  MStatus create_rig_data();
  // 执行计算
  MStatus compute();
};
}  // namespace doodle::maya_plug