/*
 * @Author: your name
 * @Date: 2020-09-29 17:22:20
 * @LastEditTime: 2020-10-09 16:23:08
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\mainWindows.h
 */
#pragma once

#include <DoodleLib/DoodleLib_fwd.h>

DOODLE_NAMESPACE_S
class mainWindows : public wxFrame {
  wxWindowIDRef p_exmaya_id;
  wxWindowIDRef p_create_image_id;
  wxWindowIDRef p_create_dir_image_id;
  wxWindowIDRef p_create_video_id;
  wxWindowIDRef p_create_ue4File_id;
  wxWindowIDRef p_mkLink_id;

 public:
  explicit mainWindows();

  DOODLE_DISABLE_COPY(mainWindows);

 private:
  std::vector<FSys::path> convertPath(const wxDropFilesEvent& event);
  void exportMayaFile(const std::vector<FSys::path>& paths);
  void createVideoFile(const std::vector<FSys::path>& paths);
  void createVideoFileFormDir(const std::vector<FSys::path>& paths);
  void connectVideo(const std::vector<FSys::path>& paths);
  void createUe4Project(const std::vector<FSys::path>& paths);
};

DOODLE_NAMESPACE_E
