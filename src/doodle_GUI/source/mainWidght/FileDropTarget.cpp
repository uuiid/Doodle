#include <doodle_GUI/source/mainWidght/FileDropTarget.h>

#include <loggerlib/Logger.h>

#include <corelib/core_Cpp.h>
#include <sstream>
namespace doodle {
bool FileDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& in_file_names) {
  std::vector<FSys::path> list{};

  const auto nums = in_file_names.size();
  for (auto i = 0; i < nums; ++i) {
    DOODLE_LOG_ERROR(in_file_names[i]);

    std::stringstream sstr{};

    DOODLE_LOG_ERROR(in_file_names[i].ToUTF8());
    DOODLE_LOG_ERROR(FSys::path{in_file_names[i]});
    sstr << in_file_names[i];
    list.emplace_back(FSys::path{sstr.str()});
  }

  // this->handleFileFunction(list);
  return true;
}
}  // namespace doodle