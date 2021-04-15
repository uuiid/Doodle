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

    // auto str = in_file_names[i].ToStdString();
    // auto buf = in_file_names[i].mb_str();
    // std::string str(buf.length(), ' ');
    // FSys::path tset{str};

    sstr << in_file_names[i];
    list.emplace_back(FSys::path{sstr.str()});
  }

  this->handleFileFunction(list);
  return true;
}
}  // namespace doodle