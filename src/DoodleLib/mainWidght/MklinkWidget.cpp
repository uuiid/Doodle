//
// Created by TD on 2021/5/10.
//

#include "MklinkWidget.h"

#include <DoodleLib/FileWarp/Ue4Project.h>
#include <DoodleLib/core/CoreSet.h>
#include <shellapi.h>


#include <boost/locale.hpp>
namespace doodle {



//MklinkWidget::MklinkWidget()
//    :{
//
//}

bool MklinkWidget::mklink(const FSys::path& in_source, const FSys::path& in_target) {
  if (FSys::exists(in_target)) {
//    wxMessageDialog{nullptr, ConvStr<wxString>("已经存在重名文件， 无法添加")}.ShowModal();
    return false;
  }

  FSys::create_directory_symlink(in_source, in_target);
  auto str      = fmt::format("完成添加:\n来源:{} \n目标:{}", in_source, in_target);
  return true;
}
}  // namespace doodle
