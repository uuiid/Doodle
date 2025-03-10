//
// Created by TD on 2021/12/13.
//

#include <doodle_core/core/file_sys.h>

#include <maya_plug/maya_plug_fwd.h>

#include "maya/MApiNamespace.h"
#include <maya/MDagPath.h>
#include <regex>

namespace doodle::maya_plug {
namespace reference_file_ns {
class generate_fbx_file_path;
}

class maya_camera {
  void chick() const;

  class regex_priority_pair {
   public:
    std::regex reg;
    std::int32_t priority;
  };

  class camera {
   public:
    MDagPath p_dag_path;
    std::int32_t priority;
    bool operator<(const camera& in_rhs) const;
    bool operator>(const camera& in_rhs) const;
    bool operator<=(const camera& in_rhs) const;
    bool operator>=(const camera& in_rhs) const;
  };

  /**
   * @brief 这个路径是相机形状的路径而不是相机变换的路径
   */
  maya_camera();
  explicit maya_camera(const MDagPath& in_path);

 public:
  MDagPath p_path;
  /**
   * @brief 烘培动画关键帧
   * @param in_start 开始时间
   * @param in_end 结束时间
   * @return 烘培完成
   */
  bool back_camera(const MTime& in_start, const MTime& in_end);

  bool camera_parent_is_word();

  bool unlock_attr();

  static maya_camera conjecture();
  void set_render_cam() const;
  void set_play_attr();
  std::double_t focalLength() const;
  std::string get_transform_name() const;
  std::string get_full_name() const;
};

}  // namespace doodle::maya_plug
