#include "export_abc_native.h"

#include <exe_maya/abc/alembic_archive_out.h>

namespace doodle::maya_plug {

void export_abc_native::export_abc(const MSelectionList& in_select, const FSys::path& in_path) {
  std::vector<MDagPath> l_dag_path{};
  l_dag_path.reserve(in_select.length());
  for (auto i = 0; i < in_select.length(); ++i) {
    MDagPath k_path{};
    in_select.getDagPath(i, k_path);
    l_dag_path.emplace_back(k_path);
  }
  alembic::archive_out l_out{in_path, l_dag_path};
}

}  // namespace doodle::maya_plug