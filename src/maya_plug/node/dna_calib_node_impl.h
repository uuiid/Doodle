#include <maya_plug/maya_plug_fwd.h>
#include <maya_plug/node/dna_calib_node.h>

#include <dnacalib/dna/DNACalibDNAReader.h>
#include <maya/MPxNode.h>

namespace doodle::maya_plug {
class dna_calib_node::impl {
 public:
  impl()  = default;
  ~impl() = default;
  dnac::ScopedPtr<dnac::FileStream> dna_file_stream_;
  dnac::ScopedPtr<dnac::BinaryStreamReader> binary_stream_reader_;
  dnac::ScopedPtr<dnac::DNACalibDNAReader> dna_calib_dna_reader_;

  MStatus open_dna_file(const FSys::path& in_path) {
    dna_file_stream_ = dnac::makeScoped<dnac::FileStream>(
        in_path.generic_string().data(), dnac::FileStream::AccessMode::ReadWrite, dnac::FileStream::OpenMode::Binary
    );
    binary_stream_reader_ = dnac::makeScoped<dnac::BinaryStreamReader>(dna_file_stream_.get());
    binary_stream_reader_->read();
    if (!dnac::Status::isOk()) return display_error("读取dna文件失败: {} ", dnac::Status::get().message), MS::kFailure;

    dna_calib_dna_reader_ = dnac::makeScoped<dnac::DNACalibDNAReader>(binary_stream_reader_.get());
    return MS::kSuccess;
  }
};
}  // namespace doodle::maya_plug