#include "doodle_batch_run.h"

#include "doodle_core/exception/exception.h"
#include "doodle_core/lib_warp/maya_exe_out.h"

#include <maya_plug/data/export_file_fbx.h>
#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/ncloth_factory.h>
#include <maya_plug/data/play_blast.h>
#include <maya_plug/data/qcloth_factory.h>

// #include <maya_plug/data/
#include <maya_plug/data/reference_file.h>
#include <maya_plug/data/sim_cover_attr.h>
#include <maya_plug/fmt/fmt_dag_path.h>

#include "data/maya_conv_str.h"
#include "data/maya_tool.h"
#include <maya/MAnimControl.h>
#include <maya/MArgDatabase.h>

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
    display_info(
        "配置布料解算完成 开始时间 {}(强制为950) 结束时间 {} 画幅 {}", anim_begin_time_.as(MTime::uiUnit()),
        MAnimControl::maxTime().as(MTime::uiUnit()), size_
    );
    maya_chick(MAnimControl::setCurrentTime(MTime{boost::numeric_cast<std::double_t>(950), MTime::uiUnit()}));
  }

  void run() {
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

  void config_export_fbx(const nlohmann::json& in_json) {
    film_aperture_     = in_json.at("camera_film_aperture").get<std::double_t>();
    size_              = in_json.at("image_size").get<image_size>();
    create_play_blast_ = in_json.at("create_play_blast").get<bool>();
    out_path_file_     = in_json.at("out_path_file").get<FSys::path>();
    anim_begin_time_   = MTime{boost::numeric_cast<std::double_t>(1001), MTime::uiUnit()};
    display_info("配置导出完成 画幅 {} 创建排屏 {}", size_, create_play_blast_);
    out_arg_.begin_time = anim_begin_time_.value();
    out_arg_.end_time   = MAnimControl::maxTime().value();
  }
  void run() {
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
    } else if (arg_data.isFlagSet(inspect_file_config)) {
    } else if (arg_data.isFlagSet(export_rig_config)) {
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