#include <doodle_GUI/source/mainWidght/FileDropTarget.h>

#include <loggerlib/Logger.h>

#include <corelib/core_Cpp.h>

namespace doodle {



bool FileDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& in_file_names) {
  std::vector<FSys::path> list{};
  for (const auto& item : in_file_names) {
    DOODLE_LOG_ERROR("drag file: " << item);
    list.emplace_back(FSys::path{ConvStr<std::string>(item)});
  }

  this->handleFileFunction(list);
  return true;
}
}  // namespace doodle