//
// AI 灯光：参考图统计层（docs/ue_ai_lighting_plan.md §5.1 的 ① 感知统计提取）
//
// 输入是**显示空间**的 8bit BGR（cv::imread 的默认输出），输出是
// reference_image_stats_t（reference.h，已锁定的数据契约）加上一个可信度标量。
//
// 为什么统计在显示空间算：§8.2 —— 参考图就是美术在显示空间里调出来的成片，
// 它的亮度分布、对比、调色板都是显示空间的量。拿线性空间的渲染去比显示空间的参考
// 必然对不上。**唯一的例外是色温**：Kelvin 是物理尺度，只在**线性光**的色度坐标上
// 才有意义，所以那一个字段内部先线性化再算（见 .cpp 里的说明）。
//
// 本模块不含任何 HTTP 依赖，单测可以直接调（与 ai_light_orchestrator 同性质）。
//
// 这组函数是**多图聚合（§5.1 的 ②）之前的单图部分**：逐图提取，不做跨图比较。
// 所以 is_outlier_ / excluded_reason_ 不在这里决定 —— 外点判定需要跨图比较。
//
// analyze_display_image 刻意做成可复用的纯函数：§8.1 的「A 目标区间满足」要拿
// 渲染图算同一组指标，那时直接调它，不要再写第二份实现。
//

#pragma once

#include <doodle_core/metadata/ai_light/reference.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <opencv2/core.hpp>

#include <optional>

namespace doodle::ai_light {

/// 单张图的统计结果 + 可信度。
struct DOODLELIB_API reference_analysis_t {
  reference_image_stats_t stats_{};
  /// 0.05..1.0。**分析成功就一定 > 0**：低置信度是一个数值，不是「没有统计」。
  /// 这条地板是 references_have_stats() 的判据（它看 stats_confidence_ > 0.0）能成立的前提 ——
  /// 否则一张纯色块会被判成「统计层没落地」，进而伪装成 prompt_only 降级。
  double confidence_{0.0};
};

/// 对**显示空间**图像（cv::imread 默认输出的 8bit BGR）计算统计。
///
/// 接受 1 / 3 / 4 通道（灰度与带 alpha 的先归一到 3 通道 BGR）；8bit 以外的深度
/// **直接抛** —— 不猜调用方的数值域是 0..255 还是 0..1。
/// 空图（empty 或 0 像素）抛 doodle_error{error_code_ = 0}，经 code_status_() 落成 500：
/// 调用方在同一个请求里刚刚成功解码过它，空图属于内部不一致，不是用户输入错误。
DOODLELIB_API reference_analysis_t analyze_display_image(const cv::Mat& in_display_bgr);

/// 读已落盘的参考图 PNG 并分析。
///
/// 读不出（文件不存在 / 解码后 0 像素）返回 nullopt —— 调用方负责翻成 400。
/// 用 IMREAD_COLOR（与 preview::convert_to_png 同一个标志）保证「落盘的文件」与
/// 「统计读的文件」是同一套解释：灰度、16bit、带 alpha 都在这里被统一成 8bit BGR。
DOODLELIB_API std::optional<reference_analysis_t> analyze_reference_file(const FSys::path& in_png);

}  // namespace doodle::ai_light
