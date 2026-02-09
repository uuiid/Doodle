#include <maya_plug/maya_plug_fwd.h>
#include <maya_plug/node/dna_calib_node.h>

#include <dnacalib/dna/DNACalibDNAReader.h>
#include <maya/MPxNode.h>

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

  MStatus open_dna_file();
};
}  // namespace doodle::maya_plug