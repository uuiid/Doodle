#include "dna_calib_node_impl.h"
namespace doodle::maya_plug {
MStatus dna_calib_node::impl_t::open_dna_file() {
  dna_file_stream_ = dnac::makeScoped<dnac::FileStream>(
      file_path_.generic_string().data(), dnac::FileStream::AccessMode::ReadWrite, dnac::FileStream::OpenMode::Binary
  );
  binary_stream_reader_ = dnac::makeScoped<dnac::BinaryStreamReader>(dna_file_stream_.get());
  binary_stream_reader_->read();
  if (!dnac::Status::isOk()) return display_error("读取dna文件失败: {} ", dnac::Status::get().message), MS::kFailure;

  dna_calib_dna_reader_ = dnac::makeScoped<dnac::DNACalibDNAReader>(binary_stream_reader_.get());
  return MS::kSuccess;
}
}  // namespace doodle::maya_plug