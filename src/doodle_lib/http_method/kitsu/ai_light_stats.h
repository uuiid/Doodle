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
// 本文件分两层：
//   1) 单图统计（§5.1 的 ①）：analyze_display_image / analyze_reference_file。逐图提取，
//      不做跨图比较 —— 所以 is_outlier_ / excluded_reason_ 不在这里决定。
//   2) 多图聚合与冲突检测（§5.1 的 ②，即 §5.3）：aggregate_reference_stats。
//      把逐图标量聚成**区间目标**，并显式标出冲突维度（**不自动平均掉**）。
//
// analyze_display_image 刻意做成可复用的纯函数：§8.1 的「A 目标区间满足」要拿
// 渲染图算同一组指标，那时直接调它，不要再写第二份实现。
//
// 为什么聚合放在这一层而不是编排层：§5.1/§5.2/§5.3 整章都是感知层，而显示空间的度量学
// 口径（含「色温要先线性化」这个唯一例外）只应该有一处定义。编排层于是只管
// 「提示词 + 目标 → LLM → Spec」这件真正的接缝工作。
//

#pragma once

#include <doodle_core/metadata/ai_light/reference.h>
#include <doodle_core/metadata/ai_light/spec.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <opencv2/core.hpp>

#include <map>
#include <optional>
#include <string>
#include <vector>

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

/// 多图聚合（§5.3）的产出：统计质量 + 由统计推出的目标区间。
struct DOODLELIB_API aggregated_reference_stats_t {
  /// image_count_ / aggregation_ / per_dimension_confidence_ / conflicts_ / retouch_detected_
  reference_stats_t stats_{};
  /// §5.3 第 2~3 步的产物：逐维度中位数 → 区间。冲突维度**仍取极值区间**，
  /// 只是同时被记进 stats_.conflicts_ 并压低置信度 —— 不自动平均掉。
  lighting_targets_t targets_{};
  /// 维度名 → 实际用到的离散度（相对或绝对，见 .cpp 的阈值表）。
  /// 落库不必要，但日志与前端可视化要靠它回答「为什么这一维被判成冲突」。
  std::map<std::string, double> dimension_spread_{};
  /// 疑似外点（只报不写，见 .cpp 的说明）。**不排除**出聚合。
  std::vector<uuid> suspected_outlier_ids_{};
};

/// 把多张参考图的**单图统计**聚合成目标区间，并检测冲突（§5.3）。
///
/// 无可用统计（全部 is_outlier_ / stats_confidence_ == 0 / stats_ 为 null）时不抛错：
/// 返回 image_count_ = 入参张数、conflicts_ 含 "no_usable_stats"、targets_ 为
/// lighting_targets_t 的默认值。**降级判定不在这里** —— 由调用方看
/// stats_.per_dimension_confidence_ 是否为空来决定 degrade_level。
///
/// 只读入参，绝不写回：is_outlier_ / excluded_reason_ 是**人工排除**字段
/// （reference_instance_route::put 写的，见 ai_light.h），自动判定写进去会让
/// 两者在同一列里不可区分。自动疑点走 suspected_outlier_ids_，由调用方持久化。
DOODLELIB_API aggregated_reference_stats_t aggregate_reference_stats(
    const std::vector<ai_light_reference>& in_references
);

}  // namespace doodle::ai_light
