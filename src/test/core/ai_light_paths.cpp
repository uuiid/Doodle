//
// AI 灯光：产物落盘路径测试
//
// 这份盯的是 kitsu_ctx_t 上那六个 get_ai_light_* 方法 —— 它们是 AI 灯光**唯一的**布局真相来源
// （HTTP 层落参考图、编排层算候选渲染图、删会话找备份目标，全都走它们）。
//
// 为什么可以直接在栈上构造 kitsu_ctx_t 而不进 g_ctx()：
// 这些方法都是头文件内联的，只读 root_ 与入参，不碰任何全局单例。
// 这一点很重要 —— test_main 里 g_ctx() 并不可用（doodle_lib::Get() 解引用的 impl::self
// 只在 app_base 构造时才被赋值，而且析构后不会清空，留下悬垂指针）。
//
// 断言一律用**字面量黄金路径**，不拿 split_uuid_path 反推：
// 反推只能证明「实现等于它自己」，钉不住 root_ 基准、ai_light 这一级、两级分桶的存在。
//

#include <doodle_core/metadata/kitsu_ctx_t.h>

#include <boost/test/unit_test.hpp>
#include <boost/uuid/string_generator.hpp>

#include <string>

namespace doodle::http {
namespace {

/// 固定的测试根。用相对路径（这些方法不做任何文件系统访问，只是拼字符串），
/// 免得把 Windows 盘符写进期望值。
const FSys::path g_root{"test_root"};

/// 固定的会话 / 轮次 / 参考图 / 候选 uuid：黄金路径要能手抄核对，不能用随机 uuid。
uuid fixed_uuid(const char* in_str) { return boost::uuids::string_generator{}(in_str); }

const char* g_session_str = "c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d";
/// 轮次 uuid：轮次目录就以它命名（不用序号）。
const char* g_iter_str = "5e6f7a8b-9c0d-4e1f-8a2b-3c4d5e6f7a8b";

kitsu_ctx_t make_ctx() {
  kitsu_ctx_t l_ctx{};
  l_ctx.root_ = g_root;
  return l_ctx;
}

}  // namespace

BOOST_AUTO_TEST_SUITE(ai_light_paths)

// ---------------------------------------------------------------------------
// 会话目录：<root>/ai_light/<uuid[0:3]>/<uuid[3:6]>/<会话 uuid>/
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(session_dir_hangs_under_root_with_two_level_fanout) {
  auto l_ctx           = make_ctx();
  const auto l_session = fixed_uuid(g_session_str);

  // uuid 的前六个字符是 "c9f2b8"：一级分桶 "c9f"、二级分桶 "2b8"，目录名是整个 uuid。
  // 黄金路径同时钉死了三件事：基准是 root_（不是项目目录）、中间必须有 ai_light 这一级、
  // 分桶必须存在 —— 任何一条被改掉，这里立刻红。
  BOOST_CHECK_EQUAL(
      l_ctx.get_ai_light_session_dir(l_session).generic_string(),
      "test_root/ai_light/c9f/2b8/c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d"
  );
}

BOOST_AUTO_TEST_CASE(session_dir_is_not_under_any_project_path) {
  auto l_ctx              = make_ctx();
  const auto l_dir        = l_ctx.get_ai_light_session_dir(fixed_uuid(g_session_str));

  // 产物落盘只依赖 root_ 与会话 uuid，**不依赖**项目/剧集/镜头路径
  // （product 是 uuid 标识的生成物，与 sd2/pictures/jobs 同性质）—— 这一条是那个边界的回归钉子。
  const std::string l_str = l_dir.generic_string();
  BOOST_CHECK_EQUAL(l_str.find("03_Workflow"), std::string::npos);
  BOOST_CHECK_EQUAL(l_str.find("Shot"), std::string::npos);
  BOOST_CHECK_EQUAL(l_dir.parent_path().parent_path().parent_path(), g_root / "ai_light");
}

// ---------------------------------------------------------------------------
// 参考图：<会话>/ref/<参考图 uuid>.png
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(reference_dir_sits_inside_the_session_dir) {
  auto l_ctx           = make_ctx();
  const auto l_session = fixed_uuid(g_session_str);
  const auto l_ref     = fixed_uuid("1a2b3c4d-5e6f-4a1b-8c2d-3e4f5a6b7c8d");

  // 回归钉子：参考图目录必须在**会话目录之内**。若做成会话目录的兄弟目录
  // （session_dir().parent_path() / "ref"），同一镜头的多个会话就共用一处 ref/，
  // 参考图会互相覆盖，删会话时也因真正的会话目录不存在而备份不到。
  const auto l_file    = l_ctx.get_ai_light_reference_file(l_session, l_ref);

  BOOST_CHECK_EQUAL(l_file.parent_path(), l_ctx.get_ai_light_reference_dir(l_session));
  BOOST_CHECK_EQUAL(
      l_ctx.get_ai_light_reference_dir(l_session).generic_string(),
      "test_root/ai_light/c9f/2b8/c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d/ref"
  );
  BOOST_CHECK_EQUAL(
      l_file.generic_string(),
      "test_root/ai_light/c9f/2b8/c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d/ref/1a2b3c4d-5e6f-4a1b-8c2d-3e4f5a6b7c8d.png"
  );
}

BOOST_AUTO_TEST_CASE(reference_files_are_named_by_their_own_uuid) {
  auto l_ctx           = make_ctx();
  const auto l_session = fixed_uuid(g_session_str);
  const auto l_a       = fixed_uuid("11111111-2222-4333-8444-555555555555");
  const auto l_b       = fixed_uuid("66666666-7777-4888-8999-aaaaaaaaaaaa");

  // 文件名必须是各自的 uuid：按序号命名（ref_00.png）一旦序号被复用就会静默覆盖既有图。
  BOOST_CHECK_NE(l_ctx.get_ai_light_reference_file(l_session, l_a), l_ctx.get_ai_light_reference_file(l_session, l_b));
  BOOST_CHECK_EQUAL(
      l_ctx.get_ai_light_reference_file(l_session, l_a).filename().generic_string(),
      "11111111-2222-4333-8444-555555555555.png"
  );
}

// ---------------------------------------------------------------------------
// 候选产物：<会话>/<轮次 uuid>/<候选 uuid>.png|.exr|.mp4 与 <候选 uuid>/ 帧目录
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(candidate_files_use_iteration_dir_and_their_own_uuid) {
  auto l_ctx           = make_ctx();
  const auto l_session = fixed_uuid(g_session_str);
  const auto l_iter_id = fixed_uuid(g_iter_str);
  const auto l_cand    = fixed_uuid("abcdefab-1234-4567-89ab-cdefabcdefab");

  // 目录名就是**轮次自己的 uuid**：候选行自带 session_id_ 与 iteration_id_，
  // 「候选 → 路径」于是是一次纯计算，不必回查轮次行拿序号。
  const auto l_iter1 = l_ctx.get_ai_light_iteration_dir(l_session, l_iter_id);
  BOOST_CHECK_EQUAL(
      l_iter1.generic_string(),
      "test_root/ai_light/c9f/2b8/c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d/5e6f7a8b-9c0d-4e1f-8a2b-3c4d5e6f7a8b"
  );
  // 轮次目录必须挂在本会话目录之下（不是与会话目录同级）。
  BOOST_CHECK_EQUAL(l_iter1.parent_path(), l_ctx.get_ai_light_session_dir(l_session));

  // 同一个候选的两张图同名不同扩展名：显示空间 PNG 给前端，线性 EXR 留工程检查（§8）。
  const auto l_png = l_ctx.get_ai_light_candidate_file(l_session, l_iter_id, l_cand, false);
  const auto l_exr = l_ctx.get_ai_light_candidate_file(l_session, l_iter_id, l_cand, true);
  BOOST_CHECK_EQUAL(l_png.parent_path(), l_iter1);
  BOOST_CHECK_EQUAL(l_png.filename().generic_string(), "abcdefab-1234-4567-89ab-cdefabcdefab.png");
  BOOST_CHECK_EQUAL(l_exr.filename().generic_string(), "abcdefab-1234-4567-89ab-cdefabcdefab.exr");
  // 默认参数就是不线性（PNG）—— 漏传时不能悄悄给出 EXR 路径。
  BOOST_CHECK_EQUAL(l_ctx.get_ai_light_candidate_file(l_session, l_iter_id, l_cand), l_png);
}

BOOST_AUTO_TEST_CASE(candidate_movie_and_frames_dir_are_named_by_their_own_uuid) {
  auto l_ctx           = make_ctx();
  const auto l_session = fixed_uuid(g_session_str);
  const auto l_iter_id = fixed_uuid(g_iter_str);
  const auto l_cand    = fixed_uuid("abcdefab-1234-4567-89ab-cdefabcdefab");

  const auto l_iter   = l_ctx.get_ai_light_iteration_dir(l_session, l_iter_id);
  const auto l_movie  = l_ctx.get_ai_light_candidate_movie_file(l_session, l_iter_id, l_cand);
  const auto l_frames = l_ctx.get_ai_light_candidate_frames_dir(l_session, l_iter_id, l_cand);

  // 一段视频一个文件：`<候选 uuid>.mp4`，与首帧海报同级、同 uuid
  BOOST_CHECK_EQUAL(l_movie.parent_path(), l_iter);
  BOOST_CHECK_EQUAL(l_movie.filename().generic_string(), "abcdefab-1234-4567-89ab-cdefabcdefab.mp4");
  // MRQ 的逐帧 PNG 落在候选自己的子目录里：一轮 6 个候选各上百张帧，平铺在轮次目录下
  // 会和 `.mp4`/`.png` 混成一团，人也没法看
  BOOST_CHECK_EQUAL(l_frames.parent_path(), l_iter);
  BOOST_CHECK_EQUAL(l_frames.filename().generic_string(), "abcdefab-1234-4567-89ab-cdefabcdefab");
  BOOST_CHECK_EQUAL(l_movie.parent_path(), l_frames.parent_path());
  BOOST_CHECK_NE(l_frames, l_movie);
}

BOOST_AUTO_TEST_CASE(iteration_dir_is_named_by_the_iteration_uuid) {
  auto l_ctx           = make_ctx();
  const auto l_session = fixed_uuid(g_session_str);
  const auto l_iter_a  = fixed_uuid(g_iter_str);
  const auto l_iter_b  = fixed_uuid("00000000-1111-4222-8333-444444444444");

  // 每一轮一个目录，名字就是它的 uuid。用序号（it_001）会要求路径反查「轮次表里那个
  // 可变的 iteration_ 字段」，uuid 则自带在候选行上。
  BOOST_CHECK_EQUAL(l_ctx.get_ai_light_iteration_dir(l_session, l_iter_a).filename().generic_string(), g_iter_str);
  BOOST_CHECK_NE(
      l_ctx.get_ai_light_iteration_dir(l_session, l_iter_a), l_ctx.get_ai_light_iteration_dir(l_session, l_iter_b)
  );
  // 候选目录属于会话目录，不跨会话串（两个会话的同一轮次 uuid 相同也不能撞在一起，
  // —— uuid 理论上不会相同，但这里钉的是「会话目录这一级必须参与」）。
  BOOST_CHECK_NE(
      l_ctx.get_ai_light_iteration_dir(l_session, l_iter_a),
      l_ctx.get_ai_light_iteration_dir(fixed_uuid("00000000-1111-4222-8333-444444444444"), l_iter_a)
  );
}

// ---------------------------------------------------------------------------
// 备份目标：<root>/ai_light/backup/<时间戳>/<会话 uuid>/
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(backup_dir_is_under_the_same_root_as_the_source) {
  auto l_ctx           = make_ctx();
  const auto l_session = fixed_uuid(g_session_str);

  const auto l_dst     = l_ctx.get_ai_light_backup_dir(l_session, "2026_09_28_11");

  // 源与备份必须同一个根：否则 root_ 在本地盘、项目在 NAS 时 FSys::rename 会跨卷失败，
  // 而失败被 catch 吞掉之后表现为「删了会话但产物没备份」。
  const auto l_src     = l_ctx.get_ai_light_session_dir(l_session);
  BOOST_CHECK_EQUAL(l_src.parent_path().parent_path().parent_path(), l_dst.parent_path().parent_path().parent_path());
  BOOST_CHECK_EQUAL(
      l_dst.generic_string(), "test_root/ai_light/backup/2026_09_28_11/c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d"
  );
  // 时间戳由调用方给：同一个会话在不同小时删，落到不同的备份目录，不会互相覆盖。
  BOOST_CHECK_NE(l_dst, l_ctx.get_ai_light_backup_dir(l_session, "2026_09_28_12"));
}

// ---------------------------------------------------------------------------
// 整体：六个方法拼出来的是同一棵子树
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(all_ai_light_artifacts_share_one_session_subtree) {
  auto l_ctx               = make_ctx();
  const auto l_session     = fixed_uuid(g_session_str);
  const auto l_iter_id     = fixed_uuid(g_iter_str);
  const auto l_ref         = fixed_uuid("1a2b3c4d-5e6f-4a1b-8c2d-3e4f5a6b7c8d");
  const auto l_cand        = fixed_uuid("abcdefab-1234-4567-89ab-cdefabcdefab");
  const auto l_session_dir = l_ctx.get_ai_light_session_dir(l_session);

  // 参考图与候选都在会话目录之下：删会话时整个目录搬进备份区就能一次搬干净，
  // 不需要知道里面有哪些文件。
  for (const auto& l_path :
       {l_ctx.get_ai_light_reference_file(l_session, l_ref),
        l_ctx.get_ai_light_candidate_file(l_session, l_iter_id, l_cand, false),
        l_ctx.get_ai_light_candidate_file(l_session, l_iter_id, l_cand, true),
        l_ctx.get_ai_light_candidate_movie_file(l_session, l_iter_id, l_cand),
        l_ctx.get_ai_light_candidate_frames_dir(l_session, l_iter_id, l_cand)}) {
    BOOST_CHECK(l_path.generic_string().starts_with(l_session_dir.generic_string() + "/"));
  }
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace doodle::http
