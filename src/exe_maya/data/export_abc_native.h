//
// Created by td_main on 2023/4/27.
//

#pragma once
#include <maya_plug/data/export_file_abc.h>
#include <maya_plug/maya_plug_fwd.h>

namespace doodle::maya_plug {
class reference_file;
class export_abc_native : public export_file_abc {
 protected:
  void export_abc(const MSelectionList& in_select, const FSys::path& in_path) override;

 public:
  using export_file_abc::export_file_abc;
};

}  // namespace doodle::maya_plug
