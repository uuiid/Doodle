#include "doodle_batch_run.h"

#include "doodle_core/exception/exception.h"
#include "doodle_core/lib_warp/maya_exe_out.h"

#include <maya_plug/data/export_file_fbx.h>
#include <maya_plug/data/maya_call_guard.h>
#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/ncloth_factory.h>
#include <maya_plug/data/play_blast.h>
#include <maya_plug/data/qcloth_factory.h>
#include <maya_plug/maya_comm/file_info_edit.h>
#include <maya_plug/node/files_info.h>

// #include <maya_plug/data/
#include "maya_plug/exception/exception.h"
#include <maya_plug/data/m_namespace.h>
#include <maya_plug/data/maya_clear_scenes.h>
#include <maya_plug/data/maya_conv_str.h>
#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/maya_tool.h>
#include <maya_plug/data/reference_file.h>
#include <maya_plug/data/sim_cover_attr.h>
#include <maya_plug/fmt/fmt_dag_path.h>
#include <maya_plug/fmt/fmt_select_list.h>
#include <maya_plug/fmt/fmt_warp.h>

#include <maya/MAnimControl.h>
#include <maya/MApiNamespace.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MFileObject.h>
#include <maya/MFn.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMesh.h>
#include <maya/MFnReference.h>
#include <maya/MFnTransform.h>
#include <maya/MItDag.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MNamespace.h>
#include <maya/MSceneMessage.h>

namespace doodle::maya_plug {
namespace {

static constexpr auto cloth_sim_config{"-cloth_sim"};
static constexpr auto export_fbx_config{"-export_fbx"};
static constexpr auto replace_file_config{"-replace_file"};
static constexpr auto inspect_file_config{"-inspect_file"};
static constexpr auto export_rig_config{"-export_rig"};

}  // namespace
MSyntax doodle_batch_run_syntax() {
  MSyntax syntax;
  syntax.addFlag("-c", "-config", MSyntax::kString);
  syntax.addFlag("-clsm", cloth_sim_config, MSyntax::kString);
  syntax.addFlag("-efbx", export_fbx_config, MSyntax::kString);
  syntax.addFlag("-ref", replace_file_config, MSyntax::kString);
  syntax.addFlag("-inf", inspect_file_config, MSyntax::kString);
  syntax.addFlag("-erig", export_rig_config, MSyntax::kString);
  return syntax;
}

class cloth_sim_run {
 public:
  cloth_sim_run()  = default;
  ~cloth_sim_run() = default;
  MTime anim_begin_time_{};
  MTime t_post_time_{};
  std::double_t film_aperture_{};
  image_size size_{};
  FSys::path file_{};
  bool replace_ref_file_{};
  bool sim_file_{};
  bool export_file_{};  // 导出abc文件
  bool touch_sim_{};
  bool export_anim_file_{};
  bool create_play_blast_{};

  void config_cloth_sim(const nlohmann::json& in_json) {
    anim_begin_time_ = MTime{boost::numeric_cast<std::double_t>(1001), MTime::uiUnit()};
    t_post_time_     = MTime{boost::numeric_cast<std::double_t>(950), MTime::uiUnit()};
    in_json.at("image_size").get_to(size_);
    in_json.at("film_aperture").get_to(film_aperture_);
    auto l_sim_path = in_json.at("sim_path").get<FSys::path>();
    if (FSys::is_directory(l_sim_path)) {
      for (auto&& l_p : FSys::directory_iterator(l_sim_path)) {
        if (FSys::is_regular_file(l_p) && l_p.path().extension() == ".ma") {
          sim_file_map_[l_p.path().filename().string()] = l_p.path();
        }
      }
    }
    replace_ref_file_  = in_json.at("replace_ref_file").get<bool>();
    sim_file_          = in_json.at("sim_file").get<bool>();
    export_file_       = in_json.at("export_file").get<bool>();
    touch_sim_         = in_json.at("touch_sim").get<bool>();
    export_anim_file_  = in_json.at("export_anim_file").get<bool>();
    create_play_blast_ = in_json.at("create_play_blast").get<bool>();
    out_path_file_     = in_json.at("out_path_file").get<FSys::path>();
    file_              = in_json.at("path").get<FSys::path>();
    display_info(
        "配置布料解算完成 开始时间 {}(强制为950) 结束时间 {} 画幅 {}", anim_begin_time_.as(MTime::uiUnit()),
        MAnimControl::maxTime().as(MTime::uiUnit()), size_
    );
    maya_chick(MAnimControl::setCurrentTime(MTime{boost::numeric_cast<std::double_t>(950), MTime::uiUnit()}));
  }

  void run() {
    maya_file_io::set_workspace(file_);
    maya_file_io::open_file(file_, MFileIO::kLoadDefault);
    maya_chick(file_info_edit::delete_node_static());
    maya_chick(MGlobal::executeCommand(R"(doodle_file_info_edit;)"));
    display_warning("开始扫瞄引用");
    all_ref_files_ = reference_file_factory{}.create_ref();

    for (auto&& l_ref : all_ref_files_) {
      if (l_ref.export_group_attr() && l_ref.get_use_sim() && l_ref.has_sim_assets_file(sim_file_map_)) {
        display_info("引用文件{}准备解算", l_ref.get_abs_path());
        ref_files_.emplace_back(l_ref);
      } else {
        display_info("引用文件{}不解算", l_ref.get_abs_path());
      }
    }
    if (replace_ref_file_) replace_ref_file();
    display_info("开始解锁节点 initialShadingGroup");
    maya_chick(MGlobal::executeCommand(d_str{R"(lockNode -l false -lu false ":initialShadingGroup";)"}));
    display_info("开始创建布料");
    cloth_factory_interface l_cf{};
    if (qcloth_factory::has_cloth())
      l_cf = std::make_shared<qcloth_factory>();
    else if (ncloth_factory::has_cloth())
      l_cf = std::make_shared<ncloth_factory>();

    if (!l_cf) return;

    auto l_cloth_list_ = l_cf->create_cloth();
    std::map<std::string, reference_file> l_ref_map{};
    for (auto&& in_handle : ref_files_) {
      l_ref_map[in_handle.get_namespace()] = in_handle;
    }

    cloth_lists_.reserve(l_cloth_list_.size());
    for (auto&& in_handle : l_cloth_list_) {
      if (!l_ref_map.contains(in_handle->get_namespace())) {
        default_logger_raw()->log(
            log_loc(), level::info, "布料{}未找到对应的引用文件, 无法解算, 请查找对应的引用", in_handle->get_shape()
        );
      }
      cloth_lists_.emplace_back(in_handle);
    }
    if (replace_ref_file_) set_cloth_attr();

    if (sim_file_) {
      display_info("安排解算布料");
      sim();
    } else if (touch_sim_) {
      display_info("安排touch布料");
      touch_sim();
    }
    if (create_play_blast_) {
      display_info("安排排屏");
      play_blast();
    }
    if (export_file_) {
      display_info("安排导出abc和fbx");
      export_abc();
    }
    if (export_anim_file_) {
      display_info("安排导出动画文件");
      export_anim_file();
    }
    write_config();
  }

 private:
  std::vector<reference_file> ref_files_{};
  std::vector<reference_file> all_ref_files_{};
  std::vector<cloth_interface> cloth_lists_{};
  std::map<std::string, FSys::path> sim_file_map_{};
  std::map<reference_file, std::vector<FSys::path>> out_and_ref_file_map_{};
  FSys::path camera_path_{};
  FSys::path out_path_file_;
  maya_out_arg out_arg_{};

  void replace_ref_file() {
    display_info("开始替换引用");
    decltype(ref_files_) l_new_ref_files;
    for (auto&& l_ref : ref_files_) {
      if (l_ref.replace_sim_assets_file(sim_file_map_)) {
        display_info("引用文件 {} 替换引用成功", l_ref.get_abs_path());
        l_new_ref_files.emplace_back(l_ref);
      } else {
        display_error("引用文件 {} 替换引用失败, 不解算", l_ref.get_abs_path());
      }
    }
    ref_files_ = std::move(l_new_ref_files);
  }
  void set_cloth_attr() {
    std::map<std::string, reference_file> l_ref_map{};

    for (auto&& in_handle : ref_files_) {
      l_ref_map[in_handle.get_namespace()] = in_handle;
    }

    for (auto&& in_handle : cloth_lists_) {
      auto l_ref_h = l_ref_map[in_handle->get_namespace()];
      in_handle->add_collision(l_ref_h);     /// 添加碰撞
      in_handle->rest();                     /// 添加rest
      in_handle->cover_cloth_attr(l_ref_h);  /// 添加布料属性
      in_handle->add_field(l_ref_h);         /// 添加场力
    }
  }
  void sim() {
    display_info("开始解算");

    std::map<std::string, reference_file> l_ref_map{};

    for (auto&& in_handle : ref_files_) {
      l_ref_map[in_handle.get_namespace()] = in_handle;
    }

    ranges::for_each(cloth_lists_, [&](cloth_interface& in_handle) {
      auto l_ref_h = l_ref_map[in_handle->get_namespace()];
      in_handle->set_cache_folder(l_ref_h, true);  /// 设置缓存文件夹
    });

    /// \brief 在这里我们保存引用
    auto k_save_file = maya_file_io::work_path("ma");
    if (!FSys::exists(k_save_file)) {
      FSys::create_directories(k_save_file);
    }

    k_save_file /= maya_file_io::get_current_path().filename();
    try {
      maya_file_io::save_file(k_save_file);
      display_info("保存文件到 {}", k_save_file);
    } catch (const std::runtime_error& error) {
      display_error("无法保存文件 {} : {}", k_save_file, error.what());
    }
    const MTime k_end_time = MAnimControl::maxTime();
    for (auto&& i = t_post_time_; i <= k_end_time; ++i) {
      maya_chick(MAnimControl::setCurrentTime(i));
      display_info("解算帧 {}", i);
      ranges::for_each(cloth_lists_, [&](cloth_interface& in_handle) { in_handle->sim_cloth(); });
    }
  }
  void touch_sim() {
    display_info("开始触摸解算");

    std::map<std::string, reference_file> l_ref_map{};

    for (auto&& in_handle : ref_files_) {
      l_ref_map[in_handle.get_namespace()] = in_handle;
    }

    ranges::for_each(cloth_lists_, [&](cloth_interface& in_handle) {
      auto l_ref_h = l_ref_map[in_handle->get_namespace()];
      in_handle->set_cache_folder_read_only();  // 设置缓存为只读
    });

    const MTime k_end_time = MAnimControl::maxTime();
    for (auto&& i = t_post_time_; i <= k_end_time; ++i) {
      maya_chick(MAnimControl::setCurrentTime(i));
      display_info("测试解算帧 {}", i);
      ranges::for_each(cloth_lists_, [&](cloth_interface& in_handle) { in_handle->sim_cloth(); });
    }
  }
  void play_blast() {
    display_info("开始排屏");
    class play_blast l_p{};

    const MTime k_end_time  = MAnimControl::maxTime();

    out_arg_.movie_file_dir = l_p.play_blast_(anim_begin_time_, k_end_time, size_);
  }
  void export_abc() {
    display_info("开始导出解算fbx");
    auto l_gen             = std::make_shared<reference_file_ns::generate_abc_file_path>();
    const MTime k_end_time = MAnimControl::maxTime();
    l_gen->begin_end_time  = std::make_pair(anim_begin_time_, k_end_time);

    export_file_fbx l_ex_fbx{};
    ranges::for_each(ref_files_, [&](reference_file& in_handle) {
      l_gen->set_fbx_path(true);
      auto l_path = l_ex_fbx.export_sim(in_handle, l_gen);
      for (auto& p : l_path) out_arg_.out_file_list.emplace_back(p);
    });
  }
  void export_anim_file() {
    display_info("开始导出动画文件");
    export_file_fbx l_ex{};
    auto l_gen             = std::make_shared<reference_file_ns::generate_abc_file_path>();
    const MTime k_end_time = MAnimControl::maxTime();
    l_gen->begin_end_time  = std::make_pair(anim_begin_time_, k_end_time);
    l_gen->set_fbx_path(true);
    ranges::for_each(
        all_ref_files_ | ranges::views::filter([&](const reference_file& in_handle) -> bool {
          return ranges::find(ref_files_, in_handle) == ref_files_.end();
        }),
        [&](reference_file& in_handle) {
          if (!in_handle.is_loaded()) in_handle.load_file();
          auto l_path = l_ex.export_anim(in_handle, l_gen);
          out_arg_.out_file_list.emplace_back(l_path);
        }
    );

    // 导出相机
    camera_path_ = l_ex.export_cam(l_gen, film_aperture_);
    out_arg_.out_file_list.emplace_back(camera_path_);  // 导出相机
  }
  void write_config() {
    display_info("导出动画文件完成, 开始写出配置文件");
    out_arg_.begin_time   = anim_begin_time_.value();
    out_arg_.end_time     = MAnimControl::maxTime().value();

    nlohmann::json l_json = out_arg_;
    if (!out_path_file_.empty()) {
      if (!FSys::exists(out_path_file_.parent_path())) FSys::create_directories(out_path_file_.parent_path());
      display_info("写出配置文件 {}", out_path_file_);
      FSys::ofstream{out_path_file_} << l_json.dump(4);
    } else
      display_info("导出文件 {}", l_json.dump(4));
  }
};

class export_fbx_run {
 public:
  export_fbx_run()  = default;
  ~export_fbx_run() = default;
  maya_out_arg out_arg_{};
  FSys::path file_{};
  void config_export_fbx(const nlohmann::json& in_json) {
    film_aperture_     = in_json.at("camera_film_aperture").get<std::double_t>();
    size_              = in_json.at("image_size").get<image_size>();
    create_play_blast_ = in_json.at("create_play_blast").get<bool>();
    out_path_file_     = in_json.at("out_path_file").get<FSys::path>();
    file_              = in_json.at("path").get<FSys::path>();

    anim_begin_time_   = MTime{boost::numeric_cast<std::double_t>(1001), MTime::uiUnit()};
    display_info("配置导出完成 画幅 {} 创建排屏 {}", size_, create_play_blast_);
    out_arg_.begin_time = anim_begin_time_.value();
    out_arg_.end_time   = MAnimControl::maxTime().value();
  }
  void run() {
    maya_file_io::set_workspace(file_);
    maya_file_io::open_file(file_, MFileIO::kLoadDefault);
    maya_chick(file_info_edit::delete_node_static());
    maya_chick(MGlobal::executeCommand(R"(doodle_file_info_edit;)"));
    display_warning("开始扫瞄引用");
    ref_files_ = reference_file_factory{}.create_ref();
    export_file_fbx l_ex{};
    auto l_gen            = std::make_shared<reference_file_ns::generate_fbx_file_path>();
    l_gen->begin_end_time = {anim_begin_time_, MAnimControl::maxTime()};
    out_arg_.begin_time   = anim_begin_time_.value();
    out_arg_.end_time     = MAnimControl::maxTime().value();
    for (auto&& i : ref_files_) {
      if (i.export_group_attr()) {
        auto l_path = l_ex.export_anim(i, l_gen);
        if (!l_path.empty()) {
          out_arg_.out_file_list.emplace_back(l_path);
        }
      }
    }
    auto l_cam_path = l_ex.export_cam(l_gen, film_aperture_);

    out_arg_.out_file_list.emplace_back(l_cam_path);

    if (create_play_blast_) {
      display_info("开始排屏");
      class play_blast l_p{};

      const MTime k_end_time  = MAnimControl::maxTime();
      out_arg_.movie_file_dir = l_p.play_blast_(anim_begin_time_, k_end_time, size_);
    }

    nlohmann::json l_json = out_arg_;
    if (!out_path_file_.empty()) {
      if (!FSys::exists(out_path_file_.parent_path())) FSys::create_directories(out_path_file_.parent_path());
      FSys::ofstream{out_path_file_} << l_json.dump(4);
      display_info("写出配置文件 {}", out_path_file_);
    } else
      display_info("导出文件 {}", l_json.dump(4));
  }

 private:
  std::double_t film_aperture_{};
  image_size size_{};
  bool create_play_blast_{};
  FSys::path out_path_file_;
  MTime anim_begin_time_{};
  std::vector<reference_file> ref_files_{};
};

class replace_file_run {
 public:
  replace_file_run()  = default;
  ~replace_file_run() = default;
  void config_replace_file(const nlohmann::json& in_json) {
    in_json.at("file_list").get_to(file_list_);
    in_json.at("path").get_to(file_path_);
    display_info("配置替换引用完成");
  }
  void run() {
    display_info("开始替换引用");

    struct tmp_data {
      std::vector<std::pair<FSys::path, FSys::path>> files_;
      FSys::path file_path_;
      std::vector<std::pair<MObject, std::string>> rename_namespaces{};
    };
    tmp_data l_data{file_list_, file_path_};

    MStatus k_s{};
    {
      maya_call_guard l_guard{MSceneMessage::addCheckReferenceCallback(
          MSceneMessage::kBeforeLoadReferenceCheck,
          [](bool* retCode, const MObject& referenceNode, MFileObject& file, void* clientData) {
            auto* self        = reinterpret_cast<tmp_data*>(clientData);
            FSys::path l_name = conv::to_s(file.rawName());
            auto l_it         = std::find_if(
                self->files_.begin(), self->files_.end(),
                [&l_name](const std::pair<FSys::path, FSys::path>& in_pair) -> bool {
                  return l_name == in_pair.first.filename();
                }
            );
            if (l_it == self->files_.end()) {
              default_logger_raw()->log(log_loc(), level::info, "跳过引用文件 {}", l_name);
              *retCode = true;
              return;
            }

            MStatus k_s{};
            k_s = file.setRawFullName(conv::to_ms(l_it->second.generic_string()));
            DOODLE_MAYA_CHICK(k_s);
            *retCode = true;

            self->rename_namespaces.emplace_back(referenceNode, l_it->second.stem().generic_string());
            default_logger_raw()->log(log_loc(), level::info, "替换加载引用文件 {}", l_it->second);
          },
          &l_data
      )};
      DOODLE_LOG_INFO("开始替换引用");
      maya_file_io::open_file(file_path_);
    }
    DOODLE_LOG_INFO("开始扫瞄引用");
    ref_files_ = g_ctx().get<reference_file_factory>().create_ref();

    // rename namespace
    DOODLE_LOG_INFO("开始重命名命名空间");
    MFnReference l_fn_ref{};
    for (auto&& l_pair : l_data.rename_namespaces) {
      k_s = l_fn_ref.setObject(l_pair.first);
      DOODLE_MAYA_CHICK(k_s);
      auto l_namespace = l_fn_ref.associatedNamespace(true);
      if (l_namespace.length() == 0) continue;
      k_s = MNamespace::renameNamespace(l_namespace, conv::to_ms(l_pair.second));
      DOODLE_MAYA_CHICK(k_s);
    }

    for (auto&& l_pair : l_data.rename_namespaces) {
      file_info_edit::refresh_node(l_pair.first);
    }

    DOODLE_LOG_INFO("重命名完成");
    maya_file_io::save_file(
        maya_plug::maya_file_io::work_path("replace_file") / maya_plug::maya_file_io::get_current_path().filename()
    );
  }

 private:
  std::vector<std::pair<FSys::path, FSys::path>> file_list_{};
  FSys::path file_path_{};
  std::vector<reference_file> ref_files_{};
};

class inspect_file_run {
 public:
  inspect_file_run()  = default;
  ~inspect_file_run() = default;

  bool surface_5_{};  // 是否检测5边面
  /// 重命名检查
  bool rename_check_{};
  /// 名称长度检查
  bool name_length_check_{};
  /// 模型历史,数值,检查
  bool history_check_{};
  /// 特殊复制检查
  bool special_copy_check_{};
  /// uv正反面检查
  bool uv_check_{};
  /// 模型k帧检查
  bool kframe_check_{};
  /// 空间名称检查
  bool space_name_check_{};
  /// 只有默认相机检查
  bool only_default_camera_check_{};
  /// 多余点数检查
  bool too_many_point_check_{};
  /// 多 UV 检查
  bool multi_uv_inspection_{};

  FSys::path file_{};
  void config_inspect_file(const nlohmann::json& in_json) {
    surface_5_                 = in_json.at("surface_5").get<bool>();
    rename_check_              = in_json.at("rename_check").get<bool>();
    name_length_check_         = in_json.at("name_length_check").get<bool>();
    history_check_             = in_json.at("history_check").get<bool>();
    special_copy_check_        = in_json.at("special_copy_check").get<bool>();
    uv_check_                  = in_json.at("uv_check").get<bool>();
    kframe_check_              = in_json.at("kframe_check").get<bool>();
    space_name_check_          = in_json.at("space_name_check").get<bool>();
    only_default_camera_check_ = in_json.at("only_default_camera_check").get<bool>();
    too_many_point_check_      = in_json.at("too_many_point_check").get<bool>();
    multi_uv_inspection_       = in_json.at("multi_uv_inspection").get<bool>();
    file_                      = in_json.at("path").get<FSys::path>();
    display_info(
        "配置检查完成 是否检测5边面 {} 重命名检查 {} 名称长度检查 {} 模型历史数值检查 {} 特殊复制检查 {} uv正反面检查 "
        "{} 模型k帧检查 {} 空间名称检查 {} 只有默认相机检查 {} 多余点数检查 {} 多UV检查 {}",
        surface_5_, rename_check_, name_length_check_, history_check_, special_copy_check_, uv_check_, kframe_check_,
        space_name_check_, only_default_camera_check_, too_many_point_check_, multi_uv_inspection_
    );
  }

  void run() {
    maya_file_io::set_workspace(file_);
    maya_file_io::open_file(file_, MFileIO::kLoadDefault);
    display_info("开始检查文件");

    MStatus l_s{};
    maya_enum::maya_error_t l_e = maya_enum::maya_error_t::success;
    if (surface_5_) {
      display_info("开始检查五边面");
      MSelectionList l_select{};
      if (maya_clear_scenes::multilateral_surface(l_select)) {
        display_error("存在五边面 {}", l_select);
        l_e = maya_enum::maya_error_t::check_error;
      }
    }
    if (rename_check_) {
      display_info("开始检查重名");
      MSelectionList l_select{};
      // for (MItDag l_iter{MItDag::kDepthFirst, MFn::kTransform, &l_s}; !l_iter.isDone(); l_iter.next())
      //   l_select.add(l_iter.currentItem());

      if (maya_clear_scenes::duplicate_name(l_select)) {
        display_error("存在重名 {}", l_select);
        l_e = maya_enum::maya_error_t::check_error;
      }
    }
    if (name_length_check_) {
      display_info("开始名称长度");
      MDagPath l_dag_path{};
      MFnDagNode l_dag_node{};

      for (MItDag l_iter{MItDag::kDepthFirst, MFn::kTransform, &l_s}; !l_iter.isDone(); l_iter.next()) {
        maya_chick(l_iter.getPath(l_dag_path));
        maya_chick(l_dag_node.setObject(l_dag_path));
        auto l_name = l_dag_node.name(&l_s);
        maya_chick(l_s);
        if (l_name.length() > 45) {
          display_error("存在超长名称 {}", l_dag_path);
          l_e = maya_enum::maya_error_t::check_error;
        }
      }
    }

    if (history_check_) {
      display_info("开始检查历史记录");
      for (MItDag l_iter{MItDag::kDepthFirst, MFn::kTransform, &l_s}; !l_iter.isDone(); l_iter.next()) {
        MObject l_root = l_iter.currentItem();
        std::int32_t l_begin{};
        for (MItDependencyGraph l_dep_it{l_root, MFn::kInvalid, MItDependencyGraph::kUpstream}; !l_dep_it.isDone();
             l_dep_it.next()) {
          ++l_begin;
          if (l_begin > 2) {
            display_error("存在历史记录 {}", get_node_full_name(l_root));
            l_e = maya_enum::maya_error_t::check_error;
            break;
          }
        }
        l_begin = {0};
      }
    }

    if (special_copy_check_) {
      display_info("开始检查特殊拷贝");
      MDagPath l_dag_path{};
      // MFnDagNode l_dag_node{};
      for (MItDag l_iter{MItDag::kDepthFirst, MFn::kMesh, &l_s}; !l_iter.isDone(); l_iter.next()) {
        maya_chick(l_iter.getPath(l_dag_path));
        // maya_chick(l_dag_node.setObject(l_dag_path));
        // display_info("检查特殊拷贝 {}", l_dag_path);
        if (l_dag_path.isInstanced(&l_s)) {
          maya_chick(l_s);
          display_error("存在特殊拷贝 {}", l_dag_path);
          l_e = maya_enum::maya_error_t::check_error;
        }
      }
    }

    {
      display_info("开始检查 UV(必须存在至少一个UV集)");
      MFnMesh l_mesh{};
      for (MItDag l_iter{MItDag::kDepthFirst, MFn::kMesh, &l_s}; !l_iter.isDone(); l_iter.next()) {
        maya_chick(l_mesh.setObject(l_iter.currentItem()));
        if (l_mesh.numUVSets() == 0) {
          display_error("存在UV缺失 {}", get_node_full_name(l_mesh.object()));
          l_e = maya_enum::maya_error_t::check_error;
        }
        if (multi_uv_inspection_)
          if (l_mesh.numUVSets() > 1) {
            display_error("存在多UV {}", get_node_full_name(l_mesh.object()));
            l_e = maya_enum::maya_error_t::check_error;
          }
      }
    }

    if (true) {
      display_info("开始检查UV");
      MFnMesh l_mesh{};
      std::int32_t l_uv_id{};
      for (MItDag l_iter{MItDag::kDepthFirst, MFn::kMesh, &l_s}; !l_iter.isDone(); l_iter.next()) {
        maya_chick(l_mesh.setObject(l_iter.currentItem()));
        if (l_mesh.numUVSets() == 0) {
          display_error("存在UV缺失 {}", get_node_full_name(l_mesh.object()));
          l_e = maya_enum::maya_error_t::check_error;
        }

        auto l_uv_name = l_mesh.currentUVSetName();
        std::vector<std::int32_t> l_polygon{};
        for (auto l_i = 0; l_i < l_mesh.numPolygons(); ++l_i) {
          MIntArray l_vertices{};
          maya_chick(l_mesh.getPolygonVertices(l_i, l_vertices));
          MFloatPoint l_point_org, l_point_x, l_point_y{};
          if (l_vertices.length() < 3) {
            display_error("存在缺失的面 {}", get_node_full_name(l_mesh.object()));
            l_e = maya_enum::maya_error_t::check_error;
            continue;
          }

          for (auto&& i = 0; i < l_mesh.polygonVertexCount(l_i); ++i) {
            if (auto l_stat = l_mesh.getPolygonUVid(l_i, i, l_uv_id, &l_uv_name); l_stat.error()) {
              display_error("存在UV缺失 {} 面id: {}", get_node_full_name(l_mesh.object()), i);
              l_e = maya_enum::maya_error_t::check_error;
            }
          }
          // maya_chick(l_mesh.getPolygonUV(l_i, 0, l_point_x.x, l_point_x.y));
          // maya_chick(l_mesh.getPolygonUV(l_i, 1, l_point_org.x, l_point_org.y));
          // maya_chick(l_mesh.getPolygonUV(l_i, 2, l_point_y.x, l_point_y.y));
          // MVector l_x{l_point_x - l_point_org}, l_y{l_point_y - l_point_org};
          // if ((l_x ^ l_y) * MVector::zAxis < 0) {
          //   l_polygon.emplace_back(l_i);
          // }
        }
        // if (l_polygon.size() > 0) {
        //   display_error(
        //       "存在反面 {}:\n {}", get_node_full_name(l_mesh.object()),
        //       fmt::join(l_polygon | ranges::views::chunk(10), "\n")
        //   );
        // }
      }
    }

    if (kframe_check_) {
      display_info("开始检查模型关键帧");

      for (MItDag l_iter{MItDag::kDepthFirst, MFn::kTransform, &l_s}; !l_iter.isDone(); l_iter.next()) {
        MObject l_root = l_iter.currentItem();
        for (MItDependencyGraph l_dep_it{l_root, MFn::kAnimCurve, MItDependencyGraph::kUpstream}; !l_dep_it.isDone();
             l_dep_it.next()) {
          display_error("存在动画K帧 {}", get_node_full_name(l_root));
          l_e = maya_enum::maya_error_t::check_error;
          break;
        }
      }
    }

    if (space_name_check_) {
      display_info("开始检查空间名称");

      for (MItDag l_iter{MItDag::kDepthFirst, MFn::kTransform, &l_s}; !l_iter.isDone(); l_iter.next()) {
        auto l_full_name = get_node_full_name(l_iter.currentItem());
        auto l_sp        = m_namespace::get_namespace_from_name(l_full_name);
        if (!l_sp.empty()) {
          display_error("存在空间名称 {}", l_full_name);
          l_e = maya_enum::maya_error_t::check_error;
        }
      }
    }

    if (only_default_camera_check_) {
      display_info("开始检查默认相机");
      std::array<std::int8_t, 5> l_checks{};
      for (MItDag l_iter{MItDag::kDepthFirst, MFn::kCamera, &l_s}; !l_iter.isDone(); l_iter.next()) {
        auto l_name = get_node_full_name(l_iter.currentItem());
        // side front top persp
        l_checks[0] += l_name.starts_with("|side");
        l_checks[1] += l_name.starts_with("|front");
        l_checks[2] += static_cast<int>(l_name.starts_with("|top"));
        l_checks[3] += static_cast<int>(l_name.starts_with("|persp"));
        ++l_checks[4];
        if (l_checks[0] > 1 || l_checks[1] > 1 || l_checks[2] > 1 || l_checks[3] > 1 || l_checks[4] > 4) {
          display_error("存在多个默认相机 {}", l_name);
          l_e = maya_enum::maya_error_t::check_error;
        }
      }
    }
    if (too_many_point_check_) {
      display_info("检查多余点数");
      MFnMesh l_mesh{};
      for (MItDag l_iter{MItDag::kDepthFirst, MFn::kMesh, &l_s}; !l_iter.isDone(); l_iter.next()) {
        maya_chick(l_mesh.setObject(l_iter.currentItem()));
        auto l_num_ver = l_mesh.numVertices();
        auto l_num_uv  = l_mesh.numUVs();
        // default_logger_raw()->warn("{} {} {}", get_node_name(l_iter.currentItem()), l_num_ver, l_num_uv);
        if (l_num_ver > l_num_uv) {
          l_e = maya_enum::maya_error_t::check_error;
          display_error("存在多余点数 {}", get_node_full_name(l_iter.currentItem()));
        }
      }
    }

    if (true) {
      display_info("检查冻结变换");
      MFnTransform l_dag_node{};
      MDagPath l_dag_path{};
      for (MItDag l_iter{MItDag::kDepthFirst, MFn::kTransform, &l_s}; !l_iter.isDone(); l_iter.next()) {
        maya_chick(l_iter.getPath(l_dag_path));
        maya_chick(l_dag_node.setObject(l_dag_path));

        auto l_full_name = get_node_full_name(l_dag_path);
        std::uint32_t l_num;
        if (l_dag_path.numberOfShapesDirectlyBelow(l_num); l_num == 1)
          maya_chick(l_dag_path.extendToShape());
        else if (l_num > 1)
          maya_chick(l_dag_path.extendToShapeDirectlyBelow(0));

        auto l_translate_mat = l_dag_node.transformation();
        maya_chick(l_s);
        if (l_dag_path.hasFn(MFn::kCamera)) continue;   // 跳过相机
        if (l_dag_path.hasFn(MFn::kLight)) continue;    // 跳过灯光
        if (l_dag_path.hasFn(MFn::kLocator)) continue;  // 跳过定位器
        if (l_dag_path.hasFn(MFn::kJoint)) continue;    // 跳过骨骼

        auto l_translate = l_translate_mat.getTranslation(MSpace::kWorld, &l_s);
        maya_chick(l_s);
        std::double_t l_scale[3]{};
        maya_chick(l_translate_mat.getScale(l_scale, MSpace::kWorld));
        maya_chick(l_s);
        std::double_t l_rot[3]{};
        MTransformationMatrix::RotationOrder l_order{};
        maya_chick(l_translate_mat.getRotation(l_rot, l_order));

        if (!((std::abs(l_translate.x) < 0.0001 && std::abs(l_translate.y) < 0.0001 &&
               std::abs(l_translate.z) < 0.0001) &&
              (std::abs(l_scale[0] - 1.0) < 0.0001 && std::abs(l_scale[1] - 1.0) < 0.0001 &&
               std::abs(l_scale[2] - 1.0) < 0.0001) &&
              (std::abs(l_rot[0]) < 0.0001 && std::abs(l_rot[1]) < 0.0001 && std::abs(l_rot[2]) < 0.0001))) {
          display_error("存在未冻结变换 {}", l_full_name);
          l_e = maya_enum::maya_error_t::check_error;
        }
      }
    }

    if (l_e == maya_enum::maya_error_t::check_error) throw_exception(doodle_error{enum_to_num(l_e), "检查文件失败"});
  }
};

class export_rig_run {
 public:
  export_rig_run()  = default;
  ~export_rig_run() = default;

  FSys::path file_{};
  FSys::path out_path_file_{};
  MTime anim_begin_time_{};

  void config_export_rig(const nlohmann::json& in_json) {
    anim_begin_time_ = MTime{boost::numeric_cast<std::double_t>(1001), MTime::uiUnit()};
    file_            = in_json.at("path").get<FSys::path>();
    out_path_file_   = in_json.at("out_path_file").get<FSys::path>();
    display_info("配置导出完成");
  }
  void run() {
    if (file_.empty()) return;

    maya_chick(MGlobal::executeCommand(R"(loadPlugin "fbxmaya";)"));

    maya_file_io::set_workspace(file_);
    maya_file_io::open_file(file_, MFileIO::kLoadDefault);

    DOODLE_LOG_INFO("开始导出 rig fbx");
    auto l_s = boost::asio::make_strand(g_io_context());
    maya_chick(MGlobal::executeCommand(R"(doodle_file_info_edit;)"));
    anim_begin_time_ = MTime{boost::numeric_cast<std::double_t>(1001), MTime::uiUnit()};

    export_file_fbx l_ex{};
    maya_out_arg l_out_arg{};
    auto l_gen            = std::make_shared<reference_file_ns::generate_fbx_file_path>();
    l_gen->begin_end_time = {anim_begin_time_, MAnimControl::maxTime()};
    l_out_arg.begin_time  = anim_begin_time_.value();
    l_out_arg.end_time    = MAnimControl::maxTime().value();
    reference_file l_ref{};
    cloth_factory_interface l_cf{};
    std::vector<cloth_interface> l_cloth_interfaces{};
    if (qcloth_factory::has_cloth()) {
      l_cf               = std::make_shared<qcloth_factory>();
      l_cloth_interfaces = l_cf->create_cloth();
    }
    auto l_out_path = l_ex.export_rig(l_ref);
    for (auto&& p : l_out_path) l_out_arg.out_file_list.emplace_back(p);
    nlohmann::json l_json = l_out_arg;
    if (!out_path_file_.empty()) {
      if (!FSys::exists(out_path_file_.parent_path())) FSys::create_directories(out_path_file_.parent_path());
      default_logger_raw()->log(log_loc(), spdlog::level::info, "写出配置文件 {}", out_path_file_);
      FSys::ofstream{out_path_file_} << l_json.dump(4);
    } else
      log_info(fmt::format("导出文件 {}", l_json.dump(4)));
  }
};

doodle_batch_run::doodle_batch_run()  = default;
doodle_batch_run::~doodle_batch_run() = default;

MStatus doodle_batch_run::doIt(const MArgList& in_list) {
  try {
    MArgDatabase arg_data(syntax(), in_list);
    auto l_config_str = arg_data.flagArgumentString("-config", 0);
    DOODLE_CHICK(l_config_str.length() == 0, "没有传入正确的配置文件路径");
    nlohmann::json l_json = nlohmann::json::parse(FSys::ifstream{FSys::from_quotation_marks(conv::to_s(l_config_str))});

    if (arg_data.isFlagSet(cloth_sim_config)) {
      cloth_sim_run l_cloth_sim{};
      l_cloth_sim.config_cloth_sim(l_json);
      l_cloth_sim.run();
    } else if (arg_data.isFlagSet(export_fbx_config)) {
      export_fbx_run l_export_fbx{};
      l_export_fbx.config_export_fbx(l_json);
      l_export_fbx.run();
    } else if (arg_data.isFlagSet(replace_file_config)) {
      replace_file_run l_replace_file{};
      l_replace_file.config_replace_file(l_json);
      l_replace_file.run();
    } else if (arg_data.isFlagSet(inspect_file_config)) {
      inspect_file_run l_inspect_file{};
      l_inspect_file.config_inspect_file(l_json);
      l_inspect_file.run();
    } else if (arg_data.isFlagSet(export_rig_config)) {
      export_rig_run l_export_rig{};
      l_export_rig.config_export_rig(l_json);
      l_export_rig.run();
    } else {
      display_error("没有传入正确的配置文件路径");
      return MStatus::kFailure;
    }
  } catch (const doodle_error& e) {
    setResult(e.error_code_);
    display_error(e.what());
    return MStatus::kSuccess;
  } catch (...) {
    setResult(-1);
    display_error(boost::current_exception_diagnostic_information());
    return MStatus::kSuccess;
  }
  return MStatus::kSuccess;
}

}  // namespace doodle::maya_plug