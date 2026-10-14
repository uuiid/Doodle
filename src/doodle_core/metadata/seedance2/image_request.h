//
// 中转站(Grsai)图片生成请求体建模 —— POST /v1/api/generate 的请求形状
//
// 数据来源(两份 Apifox 文档, 逐字段对齐):
//   * nano-banana 接口          https://qmy27nhsd9.apifox.cn/452392911e0
//   * gpt-image-2/2.5 接口      https://qmy27nhsd9.apifox.cn/452409160e0
// 与 docs/api/components/schemas/TransferStationTaskDataRequest.yaml 同构, 三者互为佐证。
//
// ─── 这个头文件是什么, 不是什么 ────────────────────────────────────────────────
// 它是**纯数据模型 + JSON 转换**: 只有结构体和 to_json/from_json, 没有任何网络、
// 落库、路径改写或校验逻辑。DB 列 `data_request` 仍然是 nlohmann::json, 本文件不提供
// 任何新的落库列; 判别这行属于哪种后端也仍然是调用方的事。
// 类型化访问的边界在客户端: transfer_station_client 提交/核算时把 data_request_ 经
// `get<transfer_station_request_t>()` 转成结构体, 改素材 URL、覆盖 replyType, 最后
// 由 to_json 重新 dump 成请求体 —— 键名从此由 to_json 保证, 不再散落在裸读键的代码里。
// 之所以要单独建模: 这份请求体是**跨进程的对外契约**(第三方服务解析它), 键名/取值拼错
// 在上游只会得到一句 "generate failed", 现场完全看不出是哪个键。把契约写成类型,
// 并由 to_json 保证「只发不带值的选填键」, 才能让错误停在编译期与单测里。
//
// ─── 为什么是结构体 + 转换函数, 而不是换掉 data_request_ 的类型 ────────────────
// 同一列 `data_request` 同时承载三种互不相干的形状: 方舟生成(model/resolution/content)、
// 中转站图片(本文件)、方舟推理 Responses(input/instructions/text.format)。历史行**没有
// 版本位**可供判别, 所以列的类型不动, 本结构体只在需要类型化访问时按 backend 显式转换:
// `data_request_.get<transfer_station_request_t>()`。后端判别由调用方按 task::backend_ 负责,
// from_json 只认它认识的键, 认不得的形状会退化成字段为空的结构体。
//
// ─── 安全性 ───────────────────────────────────────────────────────────────────
// 请求体内**没有 API key**: 密钥走 Authorization 头(Bearer sk-...), 由 ai_client_base::token_
// 持有。所以本结构体可以整体落库/打日志, 不会泄漏凭据 —— 这也是它敢作为一个「可序列化的
// 请求信封」存在的前提。
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
// 仍然显式包含 task.h: 本文件与它同属 seedance2 的任务数据模型(同样从 data_request 这一列
// 读写), 两者的宽容读法保持同一条约定 —— 历史行形状判不出来时宁可跳过, 不要抛。
#include <doodle_core/metadata/seedance2/task.h>

#include <cstdint>
#include <string>
#include <vector>

namespace doodle::seedance2 {

// ─── 为什么这里没有枚举 ─────────────────────────────────────────────────────────
// `imageSize` / `quality` / `background` / `replyType` 四个字段一律用 std::string,
// 不建枚举。理由是这四个都是**上游的自由取值域**, 而枚举的代价我们付不起:
//   * 取值随模型变、且不随本工程发版: `quality` 的支持矩阵已经按模型分成好几档
//     (gpt-image-2 仅 auto、-vip 仅 medium、.5-flare 是 low/medium/high、.5-sunburst 到 max),
//     上游加一档就得改类型重编译 —— 把「上游动了」变成「本工程要发包」;
//   * 取值本身与业务无关: `imageSize` 的 "1K" 要跟剧集授权表里的字符串**逐字一致**,
//     枚举名与 JSON 名的映射反而是多出来的一层可漂移的间接;
//   * `replyType` 本工程提交时无条件覆盖成 async, 枚举只是把它们又抄了一遍。
// 校验交给真正的事实来源: model 走定价表(g_model_pricings), 取值合法性由上游负责,
// 上游拒绝时会给出比 out_of_range.410 更有信息量的报错。
// 早先这几个字段用过 DOODLE_JSON_SERIALIZE_ENUM_STRICT 枚举, 已按上述理由回退成字符串:
// 字符串原样进 JSON, 不做大小写规整、不做白名单。

/**
 * 中转站(Grsai)图片生成请求体 —— `POST /v1/api/generate`。
 *
 * 每个字段的取值都**原样**进 JSON, 且**只有非空字段才出现**(见 to_json)。这是本结构体
 * 唯一的序列化契约, 理由有二:
 *   1) 中转站对未知键/空值的容忍度没有文档承诺。发 `"images": []` 与不发 `images`
 *      是两回事(文档的 example 里给了空数组, 但参考图的有无正是客户端分流的依据),
 *      把「未设置」与「设成空」在类型层面分开, 才不会在某个模型上偶发 400;
 *   2) 便于增量: 上游将来加字段时, 少发一个键总比多发一个空值安全。
 *
 * 素材 URL(`reference_images` / `mask`)在发送前会被改写成公网签名 URL, 见
 * ai_client_base::to_public_url 与 transfer_station_client::add_ip_to_req:
 * 绝对 URL(http/https/data/base64)原样保留, 站点内相对路径补公网前缀并签名。
 */
struct DOODLE_CORE_API transfer_station_request_t {
  /**
   * `model` —— 必填, 决定本次任务的积分消耗。
   *
   * 取值必须落在 transfer_station_client 的定价表(g_model_pricings)里: 未登记的模型
   * 在提交前就被拒(任务直接 failed, 不发下游请求), 否则会按 0 积分计费。
   * 所以**这里不做枚举约束** —— 定价表才是唯一事实来源, 在两处维护两份模型名单
   * 必然漂移。已登记模型与单价见 docs/api/components/schemas/
   * TransferStationTaskDataRequest.yaml 的 model 字段。
   */
  std::string model_;

  /// `prompt` —— 必填, 提示词
  std::string prompt_;

  /**
   * `images` —— 参考图列表, 每项是**必须的 URL**。
   *
   * 必须是 URL: 站点内相对路径(如 /api/seedance2/pictures/{uuid}.png)在发送前由
   * add_ip_to_req 补成公网签名 URL, 绝对 URL 原样保留 —— 但**不允许空串**:
   * 文档列的是 URL 或 base64(data:) 两种形态, 空串两者都不是, 上游只会报 generate failed。
   *
   * 顺序有意义: 它就是 prompt 里「图1/图2/图3」的指代顺序, 因此**不要排序、不要去重**。
   * 空 vector 表示「不发这个键」(纯文生图), 而不是发一个空数组。
   */
  std::vector<std::string> reference_images_;

  /**
   * `aspectRatio` —— 画面比例「或」像素值, 两种含义共用一个键:
   *   * nano-banana 系列        : 比例(auto/1:1/16:9/...; nano-banana-2 系列额外支持 1:4/4:1/1:8/8:1)
   *   * gpt-image-2 / 2.5       : 比例「或」1K 像素值(如 "1024x1024")
   *   * -vip / -flare / -sunburst: **只**支持 1-4K 像素值, 不支持比例
   *
   * 刻意用 string 而不是比例枚举: 自定义像素值(vip 系列允许, 约束是最大边 ≤3840、
   * 两边均为 16 的倍数、长短边比 ≤3:1、总像素 655,360 ~ 8,294,400)无论如何都表达不成枚举,
   * 而把它拆成「比例枚举 + 可选像素」两个字段又无法表达二者互斥、且 gpt-image-2 上
   * 比例与像素是同一字段的两种取值。
   */
  std::string aspect_ratio_;

  /// `imageSize` —— 1K/2K/4K, 仅 nano-banana 系列; 空表示不发该键(gpt-image 系列)
  std::string image_size_;

  /// `quality` —— 质量; 空表示不发该键(用模型默认)。必须按模型挑, 取值见上方说明
  std::string quality_;

  /// `background` —— 背景; 空表示不发该键(即不透明)。文档只列了 transparent 一个取值
  std::string background_;

  /// `mask` —— 蒙版图片地址; 空表示不发该键。与参考图同样会被改写为公网签名 URL
  std::string mask_;

  /// `replyType` —— 空表示不发该键(由中转站取默认值)。本工程提交时固定覆盖成 async
  std::string reply_type_;

  // ─── 便捷判据 ───────────────────────────────────────────────────────────────

  /// 该请求是否带参考图 —— 客户端据此在「文生图」与「改图」之间分流
  [[nodiscard]] bool has_reference_images() const { return !reference_images_.empty(); }

  friend void to_json(nlohmann::json& j, const transfer_station_request_t& p) {
    // 两个必填键无条件写: 缺了它们上游直接报错, 让它在请求体里显式可见
    // (哪怕值是空串), 比静默少一个键更容易在日志里看出来。
    j["model"]  = p.model_;
    j["prompt"] = p.prompt_;
    // 选填键: 有值才出现。判据一律是「非空/有值」, 不是「键存在」。
    if (!p.reference_images_.empty()) j["images"] = p.reference_images_;
    if (!p.aspect_ratio_.empty()) j["aspectRatio"] = p.aspect_ratio_;
    if (!p.image_size_.empty()) j["imageSize"] = p.image_size_;
    if (!p.quality_.empty()) j["quality"] = p.quality_;
    if (!p.background_.empty()) j["background"] = p.background_;
    if (!p.mask_.empty()) j["mask"] = p.mask_;
    if (!p.reply_type_.empty()) j["replyType"] = p.reply_type_;
  }

  /**
   * **严格**读法: model / prompt 必填, imageSize 与 aspectRatio 必居其一, 缺一个就抛。
   *
   * 只有这几个键是「结构完整性」, 缺了它请求发出去必然失败, 所以错要当场暴露而不是
   * 变成一个字段为空的结构体漂到上游: 上游的报错信息是一句 "generate failed",
   * 看不出是哪个键; 抛异常则带着键名和出错的 json 一起上来。
   *
   * 为什么这一列可以严格要求(它同时承载三种后端形状): 判后端是调用方的事(按
   * task::backend_), **调用点只在确认这行属于中转站之后才 get<>()**。方舟生成形态
   * (model/resolution/content)与推理形态(input/instructions)根本不该走到这里 ——
   * 它们走序列化, 不走本读法。所以「不是中转站形态」不是本函数要容忍的情形, 而是
   * 调用方误用, 应当立刻失败。
   *
   * 其余键仍是宽容的: 类型不符/缺席一律跳过并保留空串(to_json 据此不发该键),
   * 取值本身也不校验 —— 取值域在上游, 见上方「为什么这里没有枚举」。
   */
  friend void from_json(const nlohmann::json& j, transfer_station_request_t& p) {
    if (!j.is_object()) throw nlohmann::json::type_error::create(302, "中转站请求体必须是 JSON 对象", &j);

    // 必填: 两个字符串键缺一不可, 且必须是字符串
    if (!j.contains("model") || !j.at("model").is_string())
      throw nlohmann::json::type_error::create(302, "中转站请求体缺少必填的字符串键 model", &j);
    if (!j.contains("prompt") || !j.at("prompt").is_string())
      throw nlohmann::json::type_error::create(302, "中转站请求体缺少必填的字符串键 prompt", &j);
    j.at("model").get_to(p.model_);
    j.at("prompt").get_to(p.prompt_);

    // imageSize 与 aspectRatio 至少有一个: 前者是 nano-banana 的分辨率, 后者是 gpt-image 的
    // (比例或像素值)。两个都没有的话上游拿不到尺寸, 必然失败。
    const auto l_has_size   = j.contains("imageSize") && j.at("imageSize").is_string();
    const auto l_has_aspect = j.contains("aspectRatio") && j.at("aspectRatio").is_string();
    if (!l_has_size && !l_has_aspect)
      throw nlohmann::json::type_error::create(
          302, "中转站请求体必须给出 imageSize 与 aspectRatio 之一(均为字符串)", &j
      );

    if (l_has_aspect) j.at("aspectRatio").get_to(p.aspect_ratio_);
    if (l_has_size) j.at("imageSize").get_to(p.image_size_);

    // 以下仍是宽容的选填键: 类型不符/缺席一律跳过。这里**不校验取值** —— 取值域在上游。
    if (j.contains("mask") && j.at("mask").is_string()) j.at("mask").get_to(p.mask_);
    if (j.contains("images") && j.at("images").is_array()) j.at("images").get_to(p.reference_images_);
    if (j.contains("quality") && j.at("quality").is_string()) j.at("quality").get_to(p.quality_);
    if (j.contains("background") && j.at("background").is_string()) j.at("background").get_to(p.background_);
    if (j.contains("replyType") && j.at("replyType").is_string()) j.at("replyType").get_to(p.reply_type_);
  }
};

}  // namespace doodle::seedance2
