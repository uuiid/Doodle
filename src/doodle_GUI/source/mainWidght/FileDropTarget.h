#include <doodle_GUI/doodle_global.h>

#include <boost/signals2.hpp>
#include <wx/dnd.h>
namespace doodle {
class FileDropTarget : public wxFileDropTarget {
  std::vector<FSys::path> p_paths;

 public:
  virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& in_file_names) override;
  // virtual ~FileDropTarget();
  boost::signals2::signal<void(const std::vector<FSys::path>&)> handleFileFunction;
};

}  // namespace doodle