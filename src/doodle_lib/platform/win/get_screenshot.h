//
// Created by TD on 2022/5/30.
//

#include <doodle_core/platform/win/windows_alias.h>

#include <eigen3/Eigen/Core>
#include <opencv2/core.hpp>
namespace doodle::win {
/**
 * @brief 返回虚拟屏幕的坐标
 *
 * @return 左上角x,左上角y, 右下角x,右下角y
 * 0 GetSystemMetrics(SM_XVIRTUALSCREEN);
 * 1 GetSystemMetrics(SM_YVIRTUALSCREEN);
 * 2 GetSystemMetrics(SM_CXVIRTUALSCREEN);
 * 3 GetSystemMetrics(SM_CYVIRTUALSCREEN);
 */
cv::Rect2f get_system_metrics_VIRTUALSCREEN();
cv::Mat get_screenshot();

}  // namespace doodle::win
