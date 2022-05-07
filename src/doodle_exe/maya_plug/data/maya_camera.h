//
// Created by TD on 2021/12/13.
//

#include <doodle_lib/doodle_lib_fwd.h>
#include <maya/MDagPath.h>
namespace doodle::maya_plug {

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

 public:
  MDagPath p_path;
  maya_camera();
  maya_camera(const MDagPath& in_path);
  /**
   * @brief 将相机导出到文件中,
   * @param in_start 开始时间
   * @param in_end 结束时间
   * @return 如果dag path 无效则返回false
   * @throw maya_error maya返回值错误
   */
  bool export_file(const MTime& in_start, const MTime& in_end);
  /**
   * @brief 烘培动画关键帧
   * @param in_start 开始时间
   * @param in_end 结束时间
   * @return 烘培完成
   */
  bool back_camera(const MTime& in_start, const MTime& in_end);
  bool unlock_attr();

  void conjecture();
  void set_render_cam() const;
  void set_play_attr();
  std::double_t focalLength() const;
  std::string get_transform_name() const;
};

}  // namespace doodle::maya_plug
