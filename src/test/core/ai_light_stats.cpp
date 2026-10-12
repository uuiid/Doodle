//
// AI 灯光：参考图统计层测试（docs/ue_ai_lighting_plan.md §5.1 的 ①）
//
// 这份盯的是 ai_light_stats 的两个函数，**只喂合成图**：
// 不碰数据库、不碰 HTTP、不依赖任何真实素材 —— 统计的正确性必须能脱开素材复现。
// 唯一的例外是最后一个用例，它要验「读已落盘的文件」这条路径，所以会写一个临时 PNG。
//
// 为什么用合成图而不是「拿一张参考图和期望值对」：
// 期望值必须能**手算**出来，否则测试只是把当前实现的行为固化下来。
// 例如中灰 128 的三通道相等，线性化后 x/y 必然落在 sRGB 白点上 (0.3127, 0.3290)，
// 代入 McCamy 恰好是 6505K —— 这是一个能独立验证整条链的锚点，
// 公式符号写反（分母该是 0.1858 − y）会立刻在这里炸。
//

#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/ai_light/ai_light.h>

#include <doodle_lib/http_method/kitsu/ai_light_stats.h>

#include <boost/test/unit_test.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>
#include <vector>

namespace doodle::ai_light {
namespace {

/// 纯色 BGR 图（cv::Scalar 按 BGR 序，与 imread 一致）。
cv::Mat solid_bgr(int in_rows, int in_cols, const cv::Scalar& in_bgr) {
  return cv::Mat(in_rows, in_cols, CV_8UC3, in_bgr);
}

/// 水平 0..255 渐变（三通道同值）：列 c 的值 = 255·c/(cols−1)。
/// 宽度 256 时列值恰好等于 0..255，于是分位可以手算。
cv::Mat horizontal_ramp(int in_rows, int in_cols) {
  cv::Mat l_img(in_rows, in_cols, CV_8UC3);
  for (int y = 0; y < in_rows; ++y)
    for (int x = 0; x < in_cols; ++x) {
      const auto l_v = static_cast<uchar>(std::lround(255.0 * x / (in_cols - 1)));
      l_img.at<cv::Vec3b>(y, x) = cv::Vec3b(l_v, l_v, l_v);
    }
  return l_img;
}

/// 切比雪夫径向衰减：中心最亮，四条边的中点最暗（dist = 1）。
/// in_strength = 0.9 时边缘只剩 10% 亮度，足以让暗角签名满分。
cv::Mat radial_falloff(int in_rows, int in_cols, double in_strength) {
  cv::Mat l_img(in_rows, in_cols, CV_8UC3);
  const double l_cx = (in_cols - 1) / 2.0;
  const double l_cy = (in_rows - 1) / 2.0;
  for (int y = 0; y < in_rows; ++y)
    for (int x = 0; x < in_cols; ++x) {
      const double l_d = std::max(std::abs(x - l_cx) / l_cx, std::abs(y - l_cy) / l_cy);
      const auto l_v   = static_cast<uchar>(std::lround(255.0 * (1.0 - in_strength * l_d)));
      l_img.at<cv::Vec3b>(y, x) = cv::Vec3b(l_v, l_v, l_v);
    }
  return l_img;
}

/// 显式走 ADL 的 to_json。**不要**写 nlohmann::json{stats} ——
/// 花括号是列表初始化，会静默变成单元素数组（见 ai_light/common.h 顶部）。
nlohmann::json stats_to_json(const reference_image_stats_t& in_stats) {
  nlohmann::json l_json;
  to_json(l_json, in_stats);
  return l_json;
}

/// 两图的统计是否逐位相同
bool same_stats(const reference_analysis_t& in_a, const reference_analysis_t& in_b) {
  return stats_to_json(in_a.stats_) == stats_to_json(in_b.stats_);
}

}  // namespace

BOOST_AUTO_TEST_SUITE(ai_light_stats)

// ---------------------------------------------------------------------------
// 亮度分位与对比度
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(black_image_is_all_zero_and_hits_confidence_floor) {
  const auto l_res = analyze_display_image(solid_bgr(100, 100, cv::Scalar{0, 0, 0}));

  // 统计在 8bit 等级上做，所以等级 0 恰好得 0（不是「一个 bin 之内的近似值」）
  BOOST_CHECK_SMALL(l_res.stats_.luma_percentiles_.p10_, 1e-12);
  BOOST_CHECK_EQUAL(l_res.stats_.luma_percentiles_.p10_, l_res.stats_.luma_percentiles_.p90_);
  BOOST_CHECK_SMALL(l_res.stats_.contrast_, 1e-12);
  // 全黑 = 每个像素都被暗部削波
  BOOST_CHECK_CLOSE(l_res.stats_.clip_rate_.low_, 1.0, 1e-9);
  BOOST_CHECK_SMALL(l_res.stats_.clip_rate_.high_, 1e-12);
  // 中心本来就黑 → 暗角比值无意义 → 痕迹 0
  BOOST_CHECK_SMALL(l_res.stats_.retouch_score_, 1e-12);
  // 对比度 0 → content_factor 0.1，再乘分辨率因子，落到 0.05 地板
  BOOST_CHECK_CLOSE(l_res.confidence_, 0.05, 1e-9);
}

BOOST_AUTO_TEST_CASE(white_image_is_fully_clipped_high) {
  const auto l_res = analyze_display_image(solid_bgr(100, 100, cv::Scalar{255, 255, 255}));

  BOOST_CHECK_CLOSE(l_res.stats_.luma_percentiles_.p50_, 1.0, 1e-9);
  BOOST_CHECK_CLOSE(l_res.stats_.clip_rate_.high_, 1.0, 1e-9);
  BOOST_CHECK_SMALL(l_res.stats_.clip_rate_.low_, 1e-12);
  BOOST_CHECK_SMALL(l_res.stats_.contrast_, 1e-12);
  BOOST_CHECK_CLOSE(l_res.confidence_, 0.05, 1e-9);
  // 四边与中心一样亮 → 不是暗角
  BOOST_CHECK_SMALL(l_res.stats_.retouch_score_, 1e-12);
}

BOOST_AUTO_TEST_CASE(horizontal_ramp_percentiles_and_contrast) {
  // 256 列、列值 0..255、每列 64 像素 → 每个 bin 恰好 64 个样本，分位可以手算
  const auto l_res = analyze_display_image(horizontal_ramp(64, 256));

  BOOST_CHECK_SMALL(std::abs(l_res.stats_.luma_percentiles_.p10_ - 0.10), 0.01);
  BOOST_CHECK_SMALL(std::abs(l_res.stats_.luma_percentiles_.p50_ - 0.50), 0.01);
  BOOST_CHECK_SMALL(std::abs(l_res.stats_.luma_percentiles_.p90_ - 0.90), 0.01);
  // 对比度是 p90 − p10（不是标准差：见 ai_light_stats.cpp 里的说明）
  BOOST_CHECK_SMALL(std::abs(l_res.stats_.contrast_ - 0.80), 0.01);
  // 最左一列全 0、最右一列全 255 → 两端各占 1/256
  BOOST_CHECK_SMALL(std::abs(l_res.stats_.clip_rate_.low_ - 1.0 / 256.0), 1e-9);
  BOOST_CHECK_SMALL(std::abs(l_res.stats_.clip_rate_.high_ - 1.0 / 256.0), 1e-9);
  // 右边亮、左边暗：四条边不是一致变暗，所以不是暗角
  BOOST_CHECK_SMALL(l_res.stats_.retouch_score_, 1e-12);
  // 对比度 0.8 不触发「近乎纯色」分支，所以置信度高于地板
  BOOST_CHECK_GT(l_res.confidence_, 0.06);
}

// ---------------------------------------------------------------------------
// 色温：整条「sRGB → 线性光 → XYZ → xy → McCamy」链的锚点
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(neutral_gray_reads_as_d65_white_point) {
  // 中灰三通道相等 → 线性化后仍相等 → x/y 必然落在 sRGB 白点 (0.3127, 0.3290)，
  // 代入 McCamy 得 ~6505K。这个值可以独立手算，不是「看实现输出什么就断言什么」。
  const auto l_res = analyze_display_image(solid_bgr(100, 100, cv::Scalar{128, 128, 128}));

  BOOST_CHECK_SMALL(std::abs(l_res.stats_.white_balance_kelvin_ - 6500.0), 200.0);
  BOOST_CHECK_CLOSE(l_res.stats_.luma_percentiles_.p50_, 128.0 / 255.0, 1.0);
}

BOOST_AUTO_TEST_CASE(warm_and_cool_solids_are_ordered_correctly) {
  // RGB(200,150,110) → BGR(110,150,200)
  const auto l_warm = analyze_display_image(solid_bgr(100, 100, cv::Scalar{110, 150, 200}));
  BOOST_CHECK_GT(l_warm.stats_.white_balance_kelvin_, 2500.0);
  BOOST_CHECK_LT(l_warm.stats_.white_balance_kelvin_, 4200.0);

  // 冷暖对调 → 明显更冷
  const auto l_cool = analyze_display_image(solid_bgr(100, 100, cv::Scalar{200, 150, 110}));
  BOOST_CHECK_GT(l_cool.stats_.white_balance_kelvin_, l_warm.stats_.white_balance_kelvin_ + 1000.0);
}

BOOST_AUTO_TEST_CASE(saturated_primary_falls_back_instead_of_reporting_a_bogus_kelvin) {
  // 纯红永远不在普朗克轨迹附近（x ≈ 0.64）。不设 sanity 盒的话 McCamy 会给出 ~2650K ——
  // 一个看着精确、其实毫无意义的数字。要求回落到 D65 中性。
  const auto l_res = analyze_display_image(solid_bgr(100, 100, cv::Scalar{0, 0, 255}));

  BOOST_CHECK_GT(l_res.stats_.white_balance_kelvin_, 6000.0);
}

// ---------------------------------------------------------------------------
// 调色板
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(palette_has_k_entries_sorted_by_share_and_is_rgb_ordered) {
  // 5 条等宽竖条，5 个互相远离的颜色 —— 与 g_palette_k 相等，聚簇结果确定
  cv::Mat l_img(100, 200, CV_8UC3);
  const cv::Vec3b l_colors[] = {
      cv::Vec3b{0, 0, 255},      // 红
      cv::Vec3b{0, 255, 0},      // 绿
      cv::Vec3b{255, 0, 0},      // 蓝
      cv::Vec3b{0, 255, 255},    // 黄
      cv::Vec3b{255, 255, 0},    // 青
  };
  for (int y = 0; y < l_img.rows; ++y)
    for (int x = 0; x < l_img.cols; ++x) l_img.at<cv::Vec3b>(y, x) = l_colors[x / 40];

  const auto l_res = analyze_display_image(l_img);

  BOOST_REQUIRE_EQUAL(l_res.stats_.palette_.size(), 5u);
  // 占比降序、和为 1（丢簇后重新归一化过）
  double l_sum = 0.0;
  for (std::size_t i = 0; i < l_res.stats_.palette_.size(); ++i) {
    l_sum += l_res.stats_.palette_[i].share_;
    BOOST_CHECK_CLOSE(l_res.stats_.palette_[i].share_, 0.2, 1.0);
    if (i > 0) BOOST_CHECK_GE(l_res.stats_.palette_[i - 1].share_, l_res.stats_.palette_[i].share_);
  }
  BOOST_CHECK_CLOSE(l_sum, 1.0, 1e-6);

  // **通道序**：契约里的 vec3_t 是 RGB。若忘了把 k-means 中心从 BGR 换序，
  // 红色条会以 (0,0,1) 出现 —— 这两条断言就是拿来钉死这一点的。
  const auto l_is_red = [](const image_palette_entry_t& in_e) {
    return in_e.color_.x_ > 0.9 && in_e.color_.y_ < 0.1 && in_e.color_.z_ < 0.1;
  };
  const auto l_is_blue = [](const image_palette_entry_t& in_e) {
    return in_e.color_.z_ > 0.9 && in_e.color_.x_ < 0.1 && in_e.color_.y_ < 0.1;
  };
  BOOST_CHECK(std::any_of(l_res.stats_.palette_.begin(), l_res.stats_.palette_.end(), l_is_red));
  BOOST_CHECK(std::any_of(l_res.stats_.palette_.begin(), l_res.stats_.palette_.end(), l_is_blue));
}

BOOST_AUTO_TEST_CASE(analysis_is_deterministic) {
  // k-means++ 不设随机种子时每次初始化不同；这条用例钉死 cv::setRNGSeed 必须留着。
  const auto l_image = radial_falloff(64, 96, 0.5);
  const auto l_first = analyze_display_image(l_image);
  const auto l_again = analyze_display_image(l_image);

  BOOST_CHECK(same_stats(l_first, l_again));
  BOOST_CHECK_EQUAL(l_first.confidence_, l_again.confidence_);
}

// ---------------------------------------------------------------------------
// 输入形态的宽容与拒绝
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(gray_and_bgr_inputs_agree) {
  cv::Mat l_ramp_gray = horizontal_ramp(32, 128);
  cv::cvtColor(l_ramp_gray, l_ramp_gray, cv::COLOR_BGR2GRAY);

  cv::Mat l_ramp_bgr;
  cv::cvtColor(l_ramp_gray, l_ramp_bgr, cv::COLOR_GRAY2BGR);

  const auto l_from_gray = analyze_display_image(l_ramp_gray);
  const auto l_from_bgr  = analyze_display_image(l_ramp_bgr);
  BOOST_CHECK(same_stats(l_from_gray, l_from_bgr));
}

BOOST_AUTO_TEST_CASE(bgra_input_drops_alpha) {
  const auto l_bgr = horizontal_ramp(32, 128);
  cv::Mat l_bgra;
  cv::cvtColor(l_bgr, l_bgra, cv::COLOR_BGR2BGRA);

  BOOST_CHECK(same_stats(analyze_display_image(l_bgra), analyze_display_image(l_bgr)));
}

BOOST_AUTO_TEST_CASE(empty_and_non_8bit_inputs_are_rejected) {
  BOOST_CHECK_THROW(analyze_display_image(cv::Mat{}), doodle_error);

  cv::Mat l_f32;
  solid_bgr(16, 16, cv::Scalar{10, 20, 30}).convertTo(l_f32, CV_32F);
  // 8bit 与 32F 的数值域差 255 倍：这里必须抛，而不是「顺手归一化一下」
  BOOST_CHECK_THROW(analyze_display_image(l_f32), doodle_error);
}

// ---------------------------------------------------------------------------
// 后处理痕迹（暗角签名）
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(symmetric_vignette_is_detected_and_asymmetric_darkening_is_not) {
  // 径向衰减：四条边的中点都远暗于中心 → 暗角
  const auto l_vignette = analyze_display_image(radial_falloff(64, 256, 0.9));
  BOOST_CHECK_GT(l_vignette.stats_.retouch_score_, 0.5);

  // 只有左边一条竖带被压暗：四条边不是一致变暗，属于**内容**而不是暗角。
  // 这条用例是「用边中点而不是整条外缘带」的原因 —— 整条带在角上互相重叠，
  // 单侧压暗会把相邻两条边一起拉低，于是任何单侧压暗都会被误判成暗角。
  cv::Mat l_one_sided = solid_bgr(200, 200, cv::Scalar{128, 128, 128});
  l_one_sided(cv::Rect(0, 0, 20, 200)).setTo(cv::Scalar{40, 40, 40});
  const auto l_asymmetric = analyze_display_image(l_one_sided);
  BOOST_CHECK_SMALL(l_asymmetric.stats_.retouch_score_, 1e-12);
}

BOOST_AUTO_TEST_CASE(retouch_score_lowers_confidence) {
  // 两张同分辨率（256×64）的图：渐变（痕迹 0）与径向衰减（痕迹满分）。
  // 分辨率因子与 content_factor 都相同，所以置信度的差别只可能来自痕迹那一项。
  const auto l_clean    = analyze_display_image(horizontal_ramp(64, 256));
  const auto l_retouch  = analyze_display_image(radial_falloff(64, 256, 0.9));

  BOOST_CHECK_SMALL(l_clean.stats_.retouch_score_, 1e-12);
  BOOST_CHECK_GT(l_retouch.stats_.retouch_score_, 0.5);
  // 权重是 0.5，所以满痕迹正好打对折
  BOOST_CHECK_LT(l_retouch.confidence_, l_clean.confidence_ * 0.6);
  // 但不会打到地板：参考图本来就该是修过的，痕迹不该让统计「归零」
  BOOST_CHECK_GT(l_retouch.confidence_, 0.05);
}

// ---------------------------------------------------------------------------
// 置信度的分辨率因子
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(confidence_grows_with_resolution) {
  const auto l_small = analyze_display_image(horizontal_ramp(180, 320));
  const auto l_big   = analyze_display_image(horizontal_ramp(720, 1280));

  BOOST_CHECK_GT(l_big.confidence_, l_small.confidence_);
  // 1280×720 正好是基准 → 因子 1.0，配合 content_factor 1.0、痕迹 0 → 满置信度
  BOOST_CHECK_CLOSE(l_big.confidence_, 1.0, 1e-6);
}

// ---------------------------------------------------------------------------
// 读已落盘的文件
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(analyze_reference_file_reads_png_and_rejects_junk) {
  const auto l_dir = FSys::temp_directory_path();
  const auto l_png = l_dir / "doodle_test_ai_light_stats_ramp.png";
  const auto l_fake = l_dir / "doodle_test_ai_light_stats_fake.png";
  const auto l_missing = l_dir / "doodle_test_ai_light_stats_missing.png";

  const auto l_image = horizontal_ramp(64, 256);
  BOOST_REQUIRE(cv::imwrite(l_png.generic_string(), l_image));

  const auto l_from_file = analyze_reference_file(l_png);
  BOOST_REQUIRE(l_from_file.has_value());
  // PNG 无损往返，所以「读文件」与「直接喂 Mat」必须给出逐位相同的结果
  BOOST_CHECK(same_stats(*l_from_file, analyze_display_image(l_image)));

  BOOST_CHECK(!analyze_reference_file(l_missing).has_value());

  // 扩展名是 .png 但内容是文本：参考图上传路径必须能识别出这种文件，
  // 否则库里会留下一条尺寸 0×0、看起来只是「统计不可用」的坏记录。
  {
    std::ofstream l_out(l_fake, std::ios::binary);
    l_out << "definitely not a png";
  }
  BOOST_CHECK(!analyze_reference_file(l_fake).has_value());

  FSys::remove(l_png);
  FSys::remove(l_fake);
}

// ---------------------------------------------------------------------------
// 多图聚合（§5.1 的 ②，即 §5.3）
// ---------------------------------------------------------------------------
//
// 下面这些用例盯的是 aggregate_reference_stats 的三条契约 —— 它们是接缝
// （generate_spec）与后面的评判层共同依赖的前提：
//   1) 逐维取**中位数**，但落成目标的区间取**极差** —— 图之间的差异本身就是
//      「参考是近似而非精确」在数据上的表达，绝不收窄成平均值（§5.3 第 3 步）；
//   2) 冲突维度只被标记 + 压低置信度，**不清零** —— 清零的语义是「没有统计」
//      （那是 prompt_only 降级），而「有统计但互相矛盾」要交人工裁决；
//   3) 外点只报不写：suspected_outlier_ids_ 报出来，记录本身不改、也不踢出聚合。
//
// 全部喂手工构造的记录（不落库、不读文件）：聚合只读 stats_ / stats_confidence_ /
// is_outlier_ / uuid_id_ 五个字段，所以期望区间必须能手算出来。

namespace {

/// 只动第一个字节造可区分的 uuid。聚合层不看 uuid 的内容，
/// 只把它当身份标签（suspected_outlier_ids_ 里要能认出是哪一张）。
uuid make_id(unsigned char in_seed) {
  uuid l_id{};
  l_id.data[0] = in_seed;
  return l_id;
}

/// 只填标量的统计（调色板与削波留默认值），让期望区间能手算。
reference_image_stats_t make_stats(
    double in_p10, double in_p50, double in_p90, double in_contrast, double in_kelvin
) {
  reference_image_stats_t l_stats{};
  l_stats.luma_percentiles_     = luma_percentiles_value_t{in_p10, in_p50, in_p90};
  l_stats.contrast_             = in_contrast;
  l_stats.white_balance_kelvin_ = in_kelvin;
  return l_stats;
}

/// 造一条「统计层已经跑过」的记录。
ai_light_reference make_reference(const uuid& in_id, const reference_image_stats_t& in_stats, double in_confidence) {
  ai_light_reference l_ref{};
  l_ref.uuid_id_          = in_id;
  l_ref.set_stats(in_stats);
  l_ref.stats_confidence_ = in_confidence;
  return l_ref;
}

/// 置信度非零但 stats_ 仍是 null：模拟「统计层从没跑过这张图」这条路径。
ai_light_reference make_statless_reference(const uuid& in_id, double in_confidence) {
  ai_light_reference l_ref{};
  l_ref.uuid_id_          = in_id;
  l_ref.stats_confidence_ = in_confidence;
  return l_ref;
}

/// 静态断言某维度名出现在 conflicts_ 里。**先落成 bool 再交给 BOOST_TEST**：
/// Boost.Test 打不出 vector 迭代器（static_assert: has to implement operator<<）。
bool has_conflict(const std::vector<std::string>& in_conflicts, const std::string& in_key) {
  return std::find(in_conflicts.begin(), in_conflicts.end(), in_key) != in_conflicts.end();
}

/// 某字符串数组里是否含某个值（同理，避免直接比较迭代器）。
bool contains(const std::vector<std::string>& in_values, const std::string& in_key) {
  return std::find(in_values.begin(), in_values.end(), in_key) != in_values.end();
}

}  // namespace

BOOST_AUTO_TEST_CASE(aggregate_takes_median_but_keeps_range_as_interval) {
  const std::vector<ai_light_reference> l_refs{
      make_reference(make_id(1), make_stats(0.05, 0.38, 0.70, 0.66, 4000.0), 1.0),
      make_reference(make_id(2), make_stats(0.05, 0.40, 0.72, 0.70, 4000.0), 1.0),
      make_reference(make_id(3), make_stats(0.05, 0.42, 0.74, 0.74, 4000.0), 1.0),
  };
  const auto l_ret = aggregate_reference_stats(l_refs);

  BOOST_CHECK_EQUAL(l_ret.stats_.image_count_, 3);
  BOOST_CHECK_EQUAL(l_ret.stats_.aggregation_, "median");
  BOOST_TEST(!l_ret.stats_.usable_for_physical_solve_);  // 近似参考不用于物理逆解
  BOOST_CHECK_EQUAL(l_ret.targets_.space_, "display_srgb");

  // 中位数是 0.40，但区间是极差 [0.38, 0.42] —— 不是「中位数 ± 容差」。
  // 这是「多图一致范围 = 参考精度上限」在数据上的唯一表达。
  BOOST_CHECK_CLOSE(l_ret.targets_.luma_percentiles_.p50_.low_, 0.38, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.luma_percentiles_.p50_.high_, 0.42, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.luma_percentiles_.p50_.center(), 0.40, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.contrast_.low_, 0.66, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.contrast_.high_, 0.74, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.luma_percentiles_.p90_.low_, 0.70, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.luma_percentiles_.p90_.high_, 0.74, 1e-6);

  // 零宽维度（三张图 p10 一致、色温一致）按容差展开：否则评判层的
  // 「落点是否在区间内」恒为假，单张参考会让整条闭环失效。
  BOOST_CHECK_CLOSE(l_ret.targets_.luma_percentiles_.p10_.low_, 0.03, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.luma_percentiles_.p10_.high_, 0.07, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.white_balance_kelvin_.low_, 3800.0, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.white_balance_kelvin_.high_, 4200.0, 1e-6);

  // 削波是「不超过」：区间恒从 0 起；且零削波时也要留出 0.01 的**可见**余量
  // （[0,0] 会变成「一个像素都不许削」，那不是产品意图）。
  BOOST_CHECK_SMALL(l_ret.targets_.clip_rate_.high_.low_, 1e-12);
  BOOST_CHECK_CLOSE(l_ret.targets_.clip_rate_.high_.high_, 0.01, 1e-6);
  BOOST_CHECK_SMALL(l_ret.targets_.clip_rate_.low_.low_, 1e-12);
  BOOST_CHECK_CLOSE(l_ret.targets_.clip_rate_.low_.high_, 0.01, 1e-6);

  // 离散度按「极差 / max(|中位数|, 0.05)」记：冲突判据与「为什么这一维被判冲突」都读它
  BOOST_CHECK_CLOSE(l_ret.dimension_spread_.at("luma_percentiles.p50"), 0.10, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.dimension_spread_.at("contrast"), 0.08 / 0.70, 1e-6);

  // 这批图彼此一致 → 无冲突；逐维置信度齐全（接缝靠它非空判定 stats_only）
  BOOST_TEST(l_ret.stats_.conflicts_.empty());
  BOOST_TEST(!l_ret.stats_.per_dimension_confidence_.empty());
  BOOST_TEST(l_ret.stats_.per_dimension_confidence_.count("contrast") == 1);

  // 调色倾向是**可读标签**：暗角来自痕迹分（本批全 0）→ none；
  // p10 中位数 0.05 没抬黑；无削波 → 不泛光。
  BOOST_CHECK_EQUAL(l_ret.targets_.grading_hints_.vignette_, "none");
  BOOST_TEST(!l_ret.targets_.grading_hints_.lifted_blacks_);
  BOOST_CHECK_EQUAL(l_ret.targets_.grading_hints_.bloom_, "none");
  BOOST_TEST(!l_ret.stats_.retouch_detected_);

  // 没给调色板条目 → 不给调色板目标（颜色只能来自统计，不能凭空造）
  BOOST_TEST(l_ret.targets_.palette_.empty());

  // 责任归属不由 LLM 每轮重新发明（§6.6「不许改几何/材质」的落点）
  const auto& l_own = l_ret.targets_.ownership_;
  BOOST_TEST(contains(l_own.lighting_, "intensity"));
  BOOST_TEST(contains(l_own.grading_, "contrast"));
  BOOST_TEST(contains(l_own.shared_, "white_balance"));
}

BOOST_AUTO_TEST_CASE(aggregate_skips_excluded_and_statless_references) {
  auto l_excluded             = make_reference(make_id(11), make_stats(0.90, 0.90, 0.95, 0.90, 9000.0), 1.0);
  l_excluded.is_outlier_      = true;  // 与 spec_route::post 写出的语义一致
  l_excluded.excluded_reason_ = "人工排除";

  const std::vector<ai_light_reference> l_refs{
      make_reference(make_id(10), make_stats(0.05, 0.40, 0.72, 0.70, 4000.0), 1.0),
      make_reference(make_id(12), make_stats(0.05, 0.42, 0.74, 0.72, 4100.0), 1.0),
      l_excluded,
      make_reference(make_id(13), make_stats(0.01, 0.10, 0.20, 0.10, 2000.0), 0.0),  // 置信度 0
      make_statless_reference(make_id(14), 0.9),                                     // stats_ 仍是 null
  };
  const auto l_ret = aggregate_reference_stats(l_refs);

  // image_count_ 是**入参张数**（审计用），聚合只吃被三条判据筛出来的两张
  BOOST_CHECK_EQUAL(l_ret.stats_.image_count_, 5);
  BOOST_CHECK_CLOSE(l_ret.targets_.luma_percentiles_.p50_.low_, 0.40, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.luma_percentiles_.p50_.high_, 0.42, 1e-6);
  // 被人工排除的那张色温 9000K：若混进来，上界会立刻变成 9000
  BOOST_CHECK_CLOSE(l_ret.targets_.white_balance_kelvin_.low_, 4000.0, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.white_balance_kelvin_.high_, 4100.0, 1e-6);
  // 置信度 0 的那张 p10 = 0.01：若混进来，下界会变成 0.01
  BOOST_CHECK_CLOSE(l_ret.targets_.luma_percentiles_.p10_.low_, 0.05 - 0.02, 1e-6);

  // 只剩两张可用图 → 低于外点判定的 4 张下限，不报疑点
  BOOST_TEST(l_ret.suspected_outlier_ids_.empty());
  BOOST_TEST(l_ret.stats_.conflicts_.empty());
}

BOOST_AUTO_TEST_CASE(aggregate_flags_conflict_without_narrowing_or_zeroing) {
  const std::vector<ai_light_reference> l_refs{
      make_reference(make_id(21), make_stats(0.05, 0.40, 0.72, 0.70, 3200.0), 1.0),
      make_reference(make_id(22), make_stats(0.05, 0.40, 0.72, 0.70, 5600.0), 1.0),
  };
  const auto l_ret = aggregate_reference_stats(l_refs);

  BOOST_TEST(has_conflict(l_ret.stats_.conflicts_, "white_balance_kelvin"));

  // 冲突**不**收窄区间：仍是完整极差 [3200, 5600]，而不是两者平均出来的 4400。
  // 冲突只是「多贴一个标签 + 压低置信度」，取舍交给人工闸门。
  BOOST_CHECK_CLOSE(l_ret.targets_.white_balance_kelvin_.low_, 3200.0, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.white_balance_kelvin_.high_, 5600.0, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.white_balance_kelvin_.center(), 4400.0, 1e-6);

  // 冲突维度的置信度被压低，但**不下探到 0**：0 的语义是「没有统计」，
  // 那会被接缝误读成 prompt_only 降级。本维离散度远超阈值 → 落在地板 0.05。
  const double l_conf = l_ret.stats_.per_dimension_confidence_.at("white_balance_kelvin");
  BOOST_CHECK_LE(l_conf, 0.4);   // 冲突上限
  BOOST_CHECK_GE(l_conf, 0.05);  // 地板，永远 > 0
  BOOST_TEST(!l_ret.stats_.per_dimension_confidence_.empty());

  // 未冲突的维度不受牵连：两图完全一致 → 离散度 0 → 置信度全额
  BOOST_CHECK_GT(l_ret.stats_.per_dimension_confidence_.at("contrast"), 0.9);
}

BOOST_AUTO_TEST_CASE(aggregate_expands_zero_width_intervals_for_a_single_reference) {
  auto l_stats        = make_stats(0.05, 0.40, 0.72, 0.60, 4000.0);
  l_stats.clip_rate_  = clip_rate_value_t{0.03, 0.0};
  const std::vector<ai_light_reference> l_refs{make_reference(make_id(31), l_stats, 1.0)};
  const auto l_ret = aggregate_reference_stats(l_refs);

  BOOST_CHECK_EQUAL(l_ret.stats_.image_count_, 1);
  // 相对展宽优先，绝对下限兜底：p50 0.40 → max(0.10·0.40, 0.02) = 0.04
  BOOST_CHECK_CLOSE(l_ret.targets_.luma_percentiles_.p50_.low_, 0.36, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.luma_percentiles_.p50_.high_, 0.44, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.luma_percentiles_.p90_.low_, 0.648, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.luma_percentiles_.p90_.high_, 0.792, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.contrast_.low_, 0.54, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.contrast_.high_, 0.66, 1e-6);
  // 色温的相对展宽为 0（Kelvin 是物理量纲，按绝对量给）：只用 200K 下限
  BOOST_CHECK_CLOSE(l_ret.targets_.white_balance_kelvin_.low_, 3800.0, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.white_balance_kelvin_.high_, 4200.0, 1e-6);

  // 上界型维度：恒从 0 起；本图不削暗部 → 仍留 0.01 余量
  BOOST_CHECK_SMALL(l_ret.targets_.clip_rate_.high_.low_, 1e-12);
  BOOST_CHECK_CLOSE(l_ret.targets_.clip_rate_.high_.high_, 0.03, 1e-6);
  BOOST_CHECK_SMALL(l_ret.targets_.clip_rate_.low_.low_, 1e-12);
  BOOST_CHECK_CLOSE(l_ret.targets_.clip_rate_.low_.high_, 0.01, 1e-6);

  // 单图无离散度 → 不冲突；置信度齐全 → 接缝判 stats_only 而不是 prompt_only
  BOOST_TEST(l_ret.stats_.conflicts_.empty());
  BOOST_TEST(!l_ret.stats_.per_dimension_confidence_.empty());
}

BOOST_AUTO_TEST_CASE(aggregate_without_usable_stats_degrades_without_throwing) {
  auto l_excluded             = make_reference(make_id(41), make_stats(0.05, 0.40, 0.72, 0.70, 4000.0), 1.0);
  l_excluded.is_outlier_      = true;
  l_excluded.excluded_reason_ = "人工排除";

  const std::vector<ai_light_reference> l_refs{
      l_excluded,
      make_reference(make_id(42), make_stats(0.05, 0.40, 0.72, 0.70, 4000.0), 0.0),
  };
  const auto l_ret = aggregate_reference_stats(l_refs);

  BOOST_CHECK_EQUAL(l_ret.stats_.image_count_, 2);
  BOOST_TEST(has_conflict(l_ret.stats_.conflicts_, "no_usable_stats"));

  // 降级判据**留给调用方**：per_dimension_confidence_ 为空 == 没有统计 → prompt_only。
  // 聚合层自己不下这个结论（谁有权声称降级该是接缝的事，不是纯函数的事）。
  BOOST_TEST(l_ret.stats_.per_dimension_confidence_.empty());
  BOOST_TEST(l_ret.targets_.palette_.empty());
  BOOST_TEST(l_ret.dimension_spread_.empty());
  BOOST_TEST(l_ret.suspected_outlier_ids_.empty());

  // 空入参同理：不抛异常，也不伪造冲突之外的任何目标
  const std::vector<ai_light_reference> l_none{};
  const auto l_empty = aggregate_reference_stats(l_none);
  BOOST_CHECK_EQUAL(l_empty.stats_.image_count_, 0);
  BOOST_TEST(l_empty.stats_.per_dimension_confidence_.empty());
  BOOST_TEST(l_empty.targets_.palette_.empty());
}

BOOST_AUTO_TEST_CASE(aggregate_reports_suspected_outlier_without_touching_the_record) {
  const uuid l_wild_id = make_id(54);
  std::vector<ai_light_reference> l_refs{
      make_reference(make_id(51), make_stats(0.05, 0.40, 0.72, 0.70, 4000.0), 1.0),
      make_reference(make_id(52), make_stats(0.05, 0.40, 0.72, 0.70, 4000.0), 1.0),
      make_reference(make_id(53), make_stats(0.05, 0.40, 0.72, 0.70, 4000.0), 1.0),
      make_reference(l_wild_id, make_stats(0.30, 0.90, 0.95, 0.20, 9000.0), 1.0),
      make_reference(make_id(55), make_stats(0.05, 0.40, 0.72, 0.70, 4000.0), 1.0),
  };
  const auto l_ret = aggregate_reference_stats(l_refs);

  // 判据是「Σ 各维度离中位数多少个阈值」：p10 那一维就贡献了 25 格，远超 2.5。
  // 唯一被报出来的是那张异类，四张一致的图分数为 0。
  BOOST_REQUIRE_EQUAL(l_ret.suspected_outlier_ids_.size(), std::size_t{1});
  BOOST_TEST(l_ret.suspected_outlier_ids_.at(0) == l_wild_id);

  // 只报不写：疑点**仍留在聚合里** —— 区间上界仍被它拉到 0.90。
  // 「疑似」是给人工的线索（接缝 1 会把它写进提示词给模型参考），不是自动剔除。
  BOOST_CHECK_EQUAL(l_ret.stats_.image_count_, 5);
  BOOST_CHECK_CLOSE(l_ret.targets_.luma_percentiles_.p50_.high_, 0.90, 1e-6);

  // 而且聚合层绝不回写入参：is_outlier_ / excluded_reason_ 是**人工排除**字段，
  // 自动判定写进去会让两种语义在同一列里不可区分（D3）。
  BOOST_TEST(!l_refs[3].is_outlier_);
  BOOST_TEST(l_refs[3].excluded_reason_.empty());

  // 少于 4 张可用图不做外点判定：3 张里挑外点等于拿 2 张当基准，
  // 随时会把「少数派」误判成「外点」。
  const std::vector<ai_light_reference> l_three{
      make_reference(make_id(61), make_stats(0.05, 0.40, 0.72, 0.70, 4000.0), 1.0),
      make_reference(make_id(62), make_stats(0.05, 0.40, 0.72, 0.70, 4000.0), 1.0),
      make_reference(make_id(63), make_stats(0.30, 0.90, 0.95, 0.20, 9000.0), 1.0),
  };
  BOOST_TEST(aggregate_reference_stats(l_three).suspected_outlier_ids_.empty());
}

BOOST_AUTO_TEST_CASE(aggregate_palette_shares_become_cross_image_intervals) {
  auto l_a     = make_stats(0.05, 0.40, 0.72, 0.70, 4000.0);
  l_a.palette_ = {
      image_palette_entry_t{vec3_t{1.00, 0.00, 0.00}, 0.70},
      image_palette_entry_t{vec3_t{0.00, 0.00, 1.00}, 0.30},
  };
  auto l_b     = make_stats(0.05, 0.40, 0.72, 0.70, 4000.0);
  l_b.palette_ = {
      image_palette_entry_t{vec3_t{0.98, 0.01, 0.01}, 0.50},
      image_palette_entry_t{vec3_t{0.02, 0.01, 0.98}, 0.50},
  };

  const std::vector<ai_light_reference> l_refs{
      make_reference(make_id(71), l_a, 1.0),
      make_reference(make_id(72), l_b, 1.0),
  };
  const auto l_ret = aggregate_reference_stats(l_refs);

  // 4 个条目 → k = min(5, 4) = 4，每个条目各自成簇。这正是 min/max 语义该出场的地方：
  // 某张图「根本没有这个色」，占比下界就该是 0 —— 中位数会把这个 0 吃掉，
  // 于是「这个色不是每张图都有」这个事实就丢了。
  BOOST_REQUIRE_EQUAL(l_ret.targets_.palette_.size(), std::size_t{4});
  for (const auto& l_entry : l_ret.targets_.palette_) {
    BOOST_CHECK_SMALL(l_entry.share_.low_, 1e-12);
    BOOST_CHECK_GT(l_entry.share_.high_, 0.0);
    BOOST_CHECK_LE(l_entry.share_.high_, 1.0);
    // 中心必须落在 [0,1] 的 RGB 立方体里（是输入色的重聚类，不是外插）
    BOOST_CHECK_LE(l_entry.color_.x_, 1.0);
    BOOST_CHECK_GE(l_entry.color_.x_, 0.0);
    BOOST_CHECK_LE(l_entry.color_.y_, 1.0);
    BOOST_CHECK_GE(l_entry.color_.y_, 0.0);
    BOOST_CHECK_LE(l_entry.color_.z_, 1.0);
    BOOST_CHECK_GE(l_entry.color_.z_, 0.0);
  }

  // 按区间中心降序 → 「palette[0] 是主色」这条读法在单图与多图两层都成立
  BOOST_CHECK_CLOSE(l_ret.targets_.palette_.at(0).share_.center(), 0.35, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.palette_.at(0).share_.high_, 0.70, 1e-6);
  BOOST_CHECK_CLOSE(l_ret.targets_.palette_.at(3).share_.center(), 0.15, 1e-6);

  // 中心是 **RGB** 序（点集按 palette_entry_t::color_ 的 RGB 序建的，不换序）：
  // 占比最高的那一簇是 A 的纯红，所以 x 分量必须是 1 —— 把 BGR 当 RGB 用会在这里炸。
  BOOST_CHECK_CLOSE(l_ret.targets_.palette_.at(0).color_.x_, 1.0, 1e-6);
  BOOST_CHECK_SMALL(l_ret.targets_.palette_.at(0).color_.y_, 1e-6);
  BOOST_CHECK_SMALL(l_ret.targets_.palette_.at(0).color_.z_, 1e-6);

  // 主色占比跨图差 0.70 > 0.15 → 调色板冲突也要报：
  // 「主色到底是什么」本身有分歧，这是「全局调色为主」的参考里最容易被改动的量。
  BOOST_TEST(has_conflict(l_ret.stats_.conflicts_, "palette[0].share"));
  BOOST_CHECK_CLOSE(l_ret.dimension_spread_.at("palette[0].share"), 0.70, 1e-6);
  BOOST_CHECK_LE(l_ret.stats_.per_dimension_confidence_.at("palette[0].share"), 0.4);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace doodle::ai_light
