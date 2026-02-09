#include "dna_calib_node_impl.h"

#include <maya/MFnDependencyNode.h>

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
}  // namespace doodle::maya_plug