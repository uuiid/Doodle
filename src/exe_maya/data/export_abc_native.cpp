#include "export_abc_native.h"

#include "doodle_core/logger/logger.h"

#include <maya_plug/fmt/fmt_select_list.h>

#include <cmath>
#include <exe_maya/abc/alembic_archive_out.h>
#include <maya/MAnimControl.h>
#include <maya/MApiNamespace.h>
#include <vector>

namespace doodle::maya_plug {

void export_abc_native::export_abc(const MSelectionList& in_select, const FSys::path& in_path) {
  DOODLE_LOG_INFO("导出物体 {} 路径 {}", in_select, in_path);
  MAnimControl::setCurrentTime(begin_time);
  std::vector<MDagPath> l_dag_path{};
  l_dag_path.reserve(in_select.length());
  for (auto i = 0; i < in_select.length(); ++i) {
    MDagPath k_path{};
    in_select.getDagPath(i, k_path);
    l_dag_path.emplace_back(k_path);
  }

  alembic::archive_out l_out{in_path, l_dag_path, begin_time, end_time};

  for (auto&& i = begin_time; i < end_time; ++i) {
    MAnimControl::setCurrentTime(i);
    l_out.write();
  }
}

}  // namespace doodle::maya_plug