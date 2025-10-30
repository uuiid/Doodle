//
// Created by TD on 2025/9/3.
//

#include "export_xgen_abc.h"

#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/reference_file.h>
#include <maya_plug/fmt/fmt_dag_path.h>
#include <maya_plug/fmt/fmt_warp.h>

#include <Alembic/Abc/ArchiveInfo.h>
#include <Alembic/Abc/Foundation.h>
#include <Alembic/Abc/TypedArraySample.h>
#include <Alembic/AbcCoreAbstract/TimeSampling.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcGeom/FaceSetExclusivity.h>
#include <Alembic/AbcGeom/Foundation.h>
#include <Alembic/AbcGeom/GeometryScope.h>
#include <Alembic/AbcGeom/OFaceSet.h>
#include <Alembic/AbcGeom/OGeomParam.h>
#include <Alembic/AbcGeom/OPolyMesh.h>
#include <Alembic/AbcGeom/OXform.h>
#include <Alembic/AbcGeom/XformOp.h>
#include <filesystem>
#include <maya/MAnimControl.h>
#include <maya/MArgDatabase.h>
#include <maya/MDistance.h>
#include <maya/MDoubleArray.h>
#include <maya/MFn.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMesh.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MItSelectionList.h>
#include <maya/MPointArray.h>
#include <maya/MSelectionList.h>
#include <xgen/src/sggeom/SgVec3T.h>
#include <xgen/src/sggeom/SgXform3T.h>
#include <xgen/src/xgcore/XgCreator.h>
#include <xgen/src/xgcore/XgDescription.h>
#include <xgen/src/xgcore/XgGenerator.h>
#include <xgen/src/xgcore/XgPalette.h>
#include <xgen/src/xgcore/XgPatch.h>
#include <xgen/src/xgcore/XgPrimitive.h>
#include <xgen/src/xgcore/XgRenderer.h>
#include <xgen/src/xgcore/XgUtil.h>
#include <xgen/src/xgprimitive/XgArchivePrimitive.h>
#include <xgen/src/xgprimitive/XgCardPrimitive.h>
#include <xgen/src/xgprimitive/XgSpherePrimitive.h>
#include <xgen/src/xgprimitive/XgSplinePrimitive.h>
#include <xgen/src/xgrenderer/XgRenderAPI.h>
#include <xgen/src/xgrenderer/XgRenderAPIUtils.h>
namespace doodle::maya_plug {

void creare_curve(const XGenRenderAPI::vec3* in_point, std::size_t in_size) {
  MPointArray l_point_array{};
  l_point_array.setLength(in_size);
  MDoubleArray l_double_array{};
  std::double_t l_konts{boost::numeric_cast<std::double_t>(in_size + 2)};
  l_double_array.setLength(l_konts);
  for (auto j = 0; j < in_size; j++) {
    l_point_array.set(j, in_point[j].x, in_point[j].y, in_point[j].z);
    l_double_array.set(boost::numeric_cast<std::double_t>(j), j);
  }
  l_double_array.set(0.0, 1);
  l_double_array.set(0.0, 2);
  l_double_array.set(l_double_array[in_size - 1], in_size);
  l_double_array.set(l_double_array[in_size - 1], in_size + 1);

  MFnNurbsCurve l_fn{};
  MStatus l_status{};
  l_fn.create(l_point_array, l_double_array, 3, MFnNurbsCurve::Form::kOpen, false, false, MObject::kNullObj, &l_status);
  DOODLE_MAYA_CHICK(l_status);
}

class xgen_alembic_out {
 public:
  using time_sampling_ptr    = Alembic::AbcCoreAbstract::TimeSamplingPtr;
  using o_archive_ptr        = std::shared_ptr<Alembic::Abc::OArchive>;
  using o_box3d_property_ptr = std::shared_ptr<Alembic::Abc::OBox3dProperty>;
  using o_xform_ptr          = std::shared_ptr<Alembic::AbcGeom::OXform>;
  using o_curve_ptr          = std::shared_ptr<Alembic::AbcGeom::OCurves>;
  using o_curve_sample_ptr   = std::shared_ptr<Alembic::AbcGeom::OCurvesSchema::Sample>;

 private:
  MTime begin_time_{};
  MTime end_time_{};
  FSys::path out_path_{};

  o_archive_ptr o_archive_{};
  time_sampling_ptr shape_time_sampling_{};
  time_sampling_ptr transform_time_sampling_{};

  std::int32_t shape_time_index_{};
  std::int32_t transform_time_index_{};
  o_box3d_property_ptr o_box3d_property_ptr_{};

  o_xform_ptr o_xform_ptr_{};
  o_curve_ptr o_curve_ptr_{};

  struct curve_data {
    std::vector<std::int32_t> vertices_{};
    std::vector<Alembic::Abc::V3f> points_{};
    std::vector<std::float_t> widths_{};
    std::vector<std::float_t> knots_{};
    bool use_const_width_{true};
  };

  curve_data curve_data_{};
  bool init_{false};

  void open() {
    if(auto l_p = out_path_.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
    DOODLE_LOG_INFO(
        "检查到帧率 {}({}), 开始时间 {}({})", maya_plug::details::fps(), maya_plug::details::spf(), begin_time_.value(),
        end_time_.as(MTime::kSeconds)
    );
    shape_time_sampling_ = std::make_shared<Alembic::AbcCoreAbstract::TimeSampling>(
        maya_plug::details::spf(), begin_time_.as(MTime::kSeconds)
    );
    transform_time_sampling_ = std::make_shared<Alembic::AbcCoreAbstract::TimeSampling>(
        maya_plug::details::spf(), begin_time_.as(MTime::kSeconds)
    );
    o_archive_ = std::make_shared<Alembic::Abc::OArchive>(std::move(
        Alembic::Abc::v12::CreateArchiveWithInfo(
            Alembic::AbcCoreOgawa::WriteArchive{}, out_path_.generic_string(), "doodle alembic"s,
            maya_plug::maya_file_io::get_current_path().generic_string(), Alembic::Abc::ErrorHandler::kThrowPolicy
        )
    ));

    if (!o_archive_->valid()) {
      throw_exception(doodle_error{fmt::format("not open file {}", out_path_)});
    }

    shape_time_index_     = o_archive_->addTimeSampling(*shape_time_sampling_);
    transform_time_index_ = o_archive_->addTimeSampling(*transform_time_sampling_);

    o_box3d_property_ptr_ = std::make_shared<o_box3d_property_ptr::element_type>(
        std::move(Alembic::AbcGeom::CreateOArchiveBounds(*o_archive_, transform_time_index_))
    );

    if (!o_box3d_property_ptr_->valid()) {
      throw_exception(doodle_error{fmt::format("not open file {}", out_path_)});
    }
    o_xform_ptr_ =
        std::make_shared<Alembic::AbcGeom::OXform>(o_archive_->getTop(), "root_curve", transform_time_index_);
    auto& l_xform = o_xform_ptr_->getSchema();
    Alembic::AbcGeom::XformSample l_sample{};
    l_xform.set(l_sample);
    o_curve_ptr_ = std::make_shared<Alembic::AbcGeom::OCurves>(
        *o_xform_ptr_, "curve", Alembic::Abc::kFull, shape_time_index_, shape_time_sampling_
    );
  }
  static constexpr auto g_degree{3};
  void creare_curve(
      const XGenRenderAPI::vec3* in_point, std::size_t in_size, std::vector<Alembic::Abc::V3f>& out_points,
      std::vector<std::float_t>& out_knots
  ) {
    out_points.reserve(in_size + out_points.size());
    auto l_begin = out_knots.size();
    // maya的konts 是 点数 + degree  -1 = 2
    // 外部abc 是  点数 + degree  + 1 = 4
    // 一共多 4
    auto l_konts = in_size + g_degree + 1;
    auto l_end   = in_size + g_degree + 1 + out_knots.size();
    out_knots.reserve(l_end);
    /// 确保前后三个 knots 时重复的
    out_knots.emplace_back(0.f);  // 外部额外添加
    out_knots.emplace_back(0.f);
    for (auto j = 0; j < in_size; j++) {
      out_points.emplace_back(in_point[j].x, in_point[j].y, in_point[j].z);
      out_knots.emplace_back(boost::numeric_cast<std::float_t>(std::max(0, j - 1)));  // 0, 0, 1, 2
    }
    out_knots[out_knots.size() - 2] = out_knots.back();
    out_knots.emplace_back(out_knots.back());
    out_knots.emplace_back(out_knots.back());  // 外部额外添加
  }
  void creare_curve(
      const XGenRenderAPI::vec3* in_point, std::size_t in_size, std::vector<Alembic::Abc::V3f>& out_points
  ) {
    out_points.reserve(in_size + out_points.size());
    // maya的konts 是 点数 + degree  -1 = 2
    // 外部abc 是  点数 + degree  + 1 = 4
    // 一共多 4
    /// 确保前后三个 knots 时重复的
    for (auto j = 0; j < in_size; j++) {
      out_points.emplace_back(in_point[j].x, in_point[j].y, in_point[j].z);
    }
  }

 public:
  explicit xgen_alembic_out(FSys::path in_path, const MTime& in_begin_time, const MTime& in_end_time)
      : begin_time_(in_begin_time), end_time_(in_end_time), out_path_(std::move(in_path)) {
    open();
  };

  void write_begin() {
    curve_data_ = {};
    curve_data_.widths_.emplace_back();
  }

  void write_section(XGenRenderAPI::PrimitiveCache* in_cache) {
    // 写入动画
    using namespace XGenRenderAPI;
    bool bIsSpline = in_cache->get(PrimitiveCache::PrimIsSpline);
    if (!bIsSpline) return;
    if (!init_) {
      // curve_data_.use_const_width_ |= in_cache->get(PrimitiveCache::ConstWidth);
      if (curve_data_.use_const_width_) curve_data_.widths_.front() = in_cache->get(PrimitiveCache::ConstantWidth);

      auto l_num_samples = in_cache->get(PrimitiveCache::NumMotionSamples);
      // 只采样一次, 不使用运动模糊, 保留迭代, 后期可加入运动模糊
      for (auto i = 0; i < l_num_samples; i++) {
        auto* l_pos           = in_cache->get(PrimitiveCache::Points, i);
        // auto* l_width         = in_cache->get(PrimitiveCache::Widths);
        // auto l_width_size     = in_cache->getSize(PrimitiveCache::Widths);

        auto l_num            = in_cache->get(PrimitiveCache::NumVertices, i);
        const auto l_num_size = in_cache->getSize2(PrimitiveCache::NumVertices, i);
        curve_data_.points_.reserve(in_cache->getSize2(PrimitiveCache::Points, i) + curve_data_.points_.size());
        curve_data_.knots_.reserve(
            in_cache->getSize2(PrimitiveCache::Points, i) + l_num_size * 2 + curve_data_.knots_.size()
        );
        curve_data_.vertices_.reserve(l_num_size + curve_data_.vertices_.size());

        std::size_t l_index_off{};
        for (auto z = 0; z < l_num_size; ++z) {
          creare_curve(l_pos + l_index_off + 1, l_num[z] - 2, curve_data_.points_, curve_data_.knots_);
          l_index_off += l_num[z];
          curve_data_.vertices_.emplace_back(l_num[z] - 2);
        }

        break;
      }
    } else {
      auto l_num_samples = in_cache->get(PrimitiveCache::NumMotionSamples);
      // 只采样一次, 不使用运动模糊, 保留迭代, 后期可加入运动模糊
      for (auto i = 0; i < l_num_samples; i++) {
        auto* l_pos           = in_cache->get(PrimitiveCache::Points, i);
        // auto* l_width         = in_cache->get(PrimitiveCache::Widths);
        // auto l_width_size     = in_cache->getSize(PrimitiveCache::Widths);

        auto l_num            = in_cache->get(PrimitiveCache::NumVertices, i);
        const auto l_num_size = in_cache->getSize2(PrimitiveCache::NumVertices, i);
        curve_data_.points_.reserve(in_cache->getSize2(PrimitiveCache::Points, i) + curve_data_.points_.size());

        std::size_t l_index_off{};
        for (auto z = 0; z < l_num_size; ++z) {
          creare_curve(l_pos + l_index_off + 1, l_num[z] - 2, curve_data_.points_);
        }
        break;
      }
    }
  };

  void write_end() {
    Alembic::AbcGeom::OCurvesSchema::Sample l_curve_sample{};
    if (!init_) {
      l_curve_sample.setBasis(Alembic::AbcGeom::kBsplineBasis);
      l_curve_sample.setWrap(Alembic::AbcGeom::kNonPeriodic);
      l_curve_sample.setType(Alembic::AbcGeom::kCubic);
      l_curve_sample.setCurvesNumVertices(curve_data_.vertices_);
      l_curve_sample.setPositions(curve_data_.points_);
      l_curve_sample.setWidths(
          Alembic::AbcGeom::OFloatGeomParam::Sample{
              curve_data_.widths_,
              curve_data_.use_const_width_ ? Alembic::AbcGeom::kConstantScope : Alembic::AbcGeom::kVertexScope
          }
      );
      l_curve_sample.setKnots(curve_data_.knots_);
      init_ = true;
    } else {
      l_curve_sample.setPositions(curve_data_.points_);
    }
    auto& l_curve = o_curve_ptr_->getSchema();
    l_curve.set(l_curve_sample);
    Alembic::Abc::Box3d l_box{};
    for (auto&& i : curve_data_.points_) l_box.extendBy(i);
    o_box3d_property_ptr_->set(l_box);
  }
};

class XgenRender : public XGenRenderAPI::ProceduralCallbacks {
  std::string ir_render_cam_;
  std::string ir_render_cam_fov_;
  std::string ir_render_cam_xform_;
  std::string ir_render_cam_ratio_;
  std::shared_ptr<xgen_alembic_out> o_alembic_out_;

 public:
  xgen_abc_export* p_owner;
  XgenRender(xgen_abc_export* in_owner, const std::shared_ptr<xgen_alembic_out>& in_out)
      : p_owner{in_owner}, o_alembic_out_(in_out) {
    const static auto l_b_camera_ortho{false};
    const static auto l_camera_pos{SgVec3d{-48.4233, 29.8617, -21.2033}};
    const static auto l_camera_fov{54.432224};
    const static std::array<float, 16> l_camera_xform{-0.397148, 0.446873,  0.80161,  0,        5.55112e-17, 0.873446,
                                                      -0.48692,  0,         0.917755, 0.193379, 0.346887,    0,
                                                      0.228188,  -0.343197, 60.712,   1};
    const static auto l_camera_ratio{1.0};
    ir_render_cam_ = fmt::format("{},{},{},{}", l_b_camera_ortho, l_camera_pos[0], l_camera_pos[1], l_camera_pos[2]);
    ir_render_cam_fov_   = fmt::format("{}", l_camera_fov);
    ir_render_cam_xform_ = fmt::format("{}", fmt::join(l_camera_xform, ","));
    ir_render_cam_ratio_ = fmt::format("{}", l_camera_ratio);
  }

  ~XgenRender() override;
  void flush(const char* in_geom, XGenRenderAPI::PrimitiveCache* in_cache) override;
  void log(const char* in_str) override;
  bool get(EBoolAttribute) const override;
  float get(EFloatAttribute) const override;
  const char* get(EStringAttribute) const override;
  const float* get(EFloatArrayAttribute) const override;
  unsigned getSize(EFloatArrayAttribute) const override;
  const char* getOverride(const char* in_name) const override;
  /// getTransform返回给定归一化快门时间下世界空间中的当前对象变换
  void getTransform(float in_time, XGenRenderAPI::mat44& out_mat) const override;
  /**
   * getArchiveFoundingBox原始的XGen Renderman程序正在解析肋骨文件的第一行，以获取BBOX注释。
   * fscanf（fd，“##BBOX%lf%lf%lf%lf%lf”，&xmin，&xmax，&ymin，&ymax，&zmin，&zmax）；
   * 渲染器可能有更好的方法从文件名查询存档边界框。返回out_box中存档文件的边界框。（最小x，最大x，最小y，最大y，最小z，最大z）。
   * 如果成功，则返回true，否则返回false。对Proceduralcallback：：getArchiveBundingBox（）的调用在内部缓存，
   * 因此每个文件名只应调用一次该方法。可以通过调用PrimitiveCache:：getArchiveBoundingBox（）来使用缓存，
   * 该方法返回缓存的边界框，或者如果尚未调用，则使用此方法进行查询。如果返回false，则将使用默认的边界框（-0.5,0.5,0.0,1.0,-0.5,0.5）。
   *
   */
  bool getArchiveBoundingBox(const char* in_filename, XGenRenderAPI::bbox& out_bbox) const override;
};
XgenRender::~XgenRender() = default;
void XgenRender::flush(const char* in_geom, XGenRenderAPI::PrimitiveCache* in_cache) {
  o_alembic_out_->write_section(in_cache);
}
void XgenRender::log(const char* in_str) {
  p_owner->displayInfo(in_str);
  default_logger_raw()->info(in_str);
}
bool XgenRender::get(EBoolAttribute in_attr) const {
  if (in_attr == EBoolAttribute::ClearDescriptionCache) return true;  // 这样才会在运行时渲染
  return false;
}
float XgenRender::get(EFloatAttribute in_attr) const { return 0.f; }
const char* XgenRender::get(EStringAttribute in_attr) const {
  if (in_attr == EStringAttribute::CacheDir) return "xgenCache/";
  if (in_attr == EStringAttribute::RenderCam) return ir_render_cam_.c_str();
  if (in_attr == EStringAttribute::RenderCamFOV) return ir_render_cam_fov_.c_str();
  if (in_attr == EStringAttribute::RenderCamRatio) return ir_render_cam_ratio_.c_str();
  if (in_attr == EStringAttribute::RenderCamXform) return ir_render_cam_xform_.c_str();
  if (in_attr == EStringAttribute::RenderMethod) return "1";

  return "";
}
const float* XgenRender::get(EFloatArrayAttribute) const { return nullptr; }
unsigned XgenRender::getSize(EFloatArrayAttribute) const { return 0; }
const char* XgenRender::getOverride(const char* in_name) const { return ""; }         /// 确切不返回任何的覆盖属性
void XgenRender::getTransform(float in_time, XGenRenderAPI::mat44& out_mat) const {}  /// 先不进行变换
bool XgenRender::getArchiveBoundingBox(const char* in_filename, XGenRenderAPI::bbox& out_bbox) const { return false; }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct palette_warp {
  XgPalette* palette_ptr;
  MDagPath palette_dag;
};

struct xgen_abc_export::impl {
  std::vector<palette_warp> palette_v{};
  MTime begin_time_;
  MTime end_time_;
  std::string create_render_args(const palette_warp& in_, const XgDescription* in_des, const XgPatch* in_patch);
};
xgen_abc_export::xgen_abc_export() : p_i{std::make_unique<impl>()} {}
xgen_abc_export::~xgen_abc_export() = default;
MSyntax xgen_abc_export_syntax() {
  MSyntax syntax;
  syntax.addFlag("-s", "-start_time", MSyntax::kTime);
  syntax.addFlag("-e", "-end_time", MSyntax::kTime);
  syntax.addFlag("-n", "-one_time", MSyntax::kNoArg);
  syntax.setObjectType(MSyntax::kSelectionList);
  syntax.useSelectionAsDefault(true);
  return syntax;
}
MStatus xgen_abc_export::doIt(const MArgList& in_arg) {
  parse_args(in_arg);
  return redoIt();
}
std::string xgen_abc_export::impl::create_render_args(
    const palette_warp& in_, const XgDescription* in_des, const XgPatch* in_patch
) {
  auto l_current_frame = MAnimControl::currentTime();
  auto l_namespace     = get_name_space(in_.palette_dag);
  std::double_t l_unit_conv{1};  // default cm;
  switch (MDistance::uiUnit()) {
    case MDistance::kInches:
      l_unit_conv = 2.54;
      break;
    case MDistance::kFeet:
      l_unit_conv = 30.48;
      break;
    case MDistance::kYards:
      l_unit_conv = 91.44;
      break;
    case MDistance::kMiles:
      l_unit_conv = 160934.4;
      break;
    case MDistance::kMillimeters:
      l_unit_conv = 0.1;
      break;
    case MDistance::kKilometers:
      l_unit_conv = 100000.0;
      break;
    case MDistance::kMeters:
      l_unit_conv = 100.0;
      break;
    case MDistance::kCentimeters:
    case MDistance::kLast:
    case MDistance::kInvalid:
      break;
  }
  FSys::path l_file_path = maya_file_io::get_current_path();
  l_file_path.replace_extension();
  FSys::path l_geom_file_path{"D:/test_files/test_xgen/cache/alembic/cache.abc"};

  auto l_str = fmt::format(
      "-debug 1 -warning 1 -stats 1{8}{1} -palette {2} -description {3} -patch {4} -frame {5} "
      "-file {6}__{2}.xgen -geom {7} -fps {9} -interpolation linear  -motionSamplesLookup 0.0 "
      "-motionSamplesPlacement 0.0 -world {0};0;0;0;0;{0};0;0;0;0;{0};0;0;0;0;1",
      l_unit_conv, l_namespace, in_.palette_ptr->name(), in_des->name(), in_patch->name(),
      l_current_frame.as(MTime::uiUnit()), l_file_path, l_geom_file_path, !l_namespace.empty() ? " -nameSpace " : "",
      details::fps()
  );

  return l_str;
}

struct xgen_render_face {
  std::unique_ptr<XgenRender> main_render{};
  std::unique_ptr<XGenRenderAPI::PatchRenderer> patch_renderer{};
  std::map<std::uint32_t, std::unique_ptr<XGenRenderAPI::FaceRenderer>> face_render{};
};
struct xgen_render_des {
  std::shared_ptr<xgen_alembic_out> xgen_alembic_out_ptr_{};
  std::vector<std::unique_ptr<xgen_render_face>> face_list_{};
};
MStatus xgen_abc_export::redoIt() {
  auto l_abc_file_path_gen            = std::make_shared<reference_file_ns::generate_abc_file_path>();
  l_abc_file_path_gen->begin_end_time = {p_i->begin_time_, p_i->end_time_};
  std::vector<std::unique_ptr<xgen_render_des>> l_render_list{};
  // 初始化渲染器
  for (auto&& i : p_i->palette_v) {
    auto l_namespace = get_name_space(i.palette_dag);
    for (auto j = 0; j < i.palette_ptr->numDescriptions(); ++j) {
      auto l_des         = i.palette_ptr->description(j);
      auto& l_des_render = l_render_list.emplace_back(std::make_unique<xgen_render_des>());
      if (!l_des) {
        displayError(
            conv::to_ms(fmt::format("集合 {} 描述 index {} 为空(请刷新xgen预览), 无法解析", i.palette_ptr->name(), j))
        );
        continue;
      }
      l_abc_file_path_gen->add_external_string = l_des->name();

      auto l_out_path                          = (*l_abc_file_path_gen)(l_namespace);
      displayInfo(conv::to_ms(fmt::format("导出路径 {}", l_out_path)));
      l_des_render->xgen_alembic_out_ptr_ =
          std::make_shared<xgen_alembic_out>(l_out_path, p_i->begin_time_, p_i->end_time_);

      BOOST_ASSERT(l_des_render->xgen_alembic_out_ptr_);
      for (auto&& [l_path_name, l_ptr] : l_des->patches()) {
        if (!l_ptr) {
          displayError(
              conv::to_ms(
                  fmt::format(
                      "集合 {} 描述 {} patches {} 为空(请刷新xgen预览), 无法解析", i.palette_ptr->name(), l_des->name(),
                      l_path_name
                  )
              )
          );
          continue;
        }
        auto& l_render_        = l_des_render->face_list_.emplace_back(std::make_unique<xgen_render_face>());
        l_render_->main_render = std::make_unique<XgenRender>(this, l_des_render->xgen_alembic_out_ptr_);
        auto l_args            = p_i->create_render_args(i, l_des, l_ptr);
        // displayInfo(l_args.c_str());
        l_render_->patch_renderer = std::unique_ptr<XGenRenderAPI::PatchRenderer>{
            XGenRenderAPI::PatchRenderer::init(l_render_->main_render.get(), l_args.c_str())
        };
        XGenRenderAPI::bbox l_sub_bbox{};
        std::uint32_t l_face_id = -1;
        while (l_render_->patch_renderer->nextFace(l_sub_bbox, l_face_id)) {
          l_render_->face_render.emplace(
              l_face_id, XGenRenderAPI::FaceRenderer::init(l_render_->patch_renderer.get(), l_face_id)
          );
        }
      }
    }
  }

  for (auto i = p_i->begin_time_; i <= p_i->end_time_; ++i) {
    MAnimControl::setCurrentTime(i);
    for (auto&& l_list : l_render_list) {
      l_list->xgen_alembic_out_ptr_->write_begin();
      for (auto&& l_r : l_list->face_list_) {
        XGenRenderAPI::bbox l_sub_bbox{};
        std::uint32_t l_face_id = -1;
        while (l_r->patch_renderer->nextFace(l_sub_bbox, l_face_id)) l_r->face_render.at(l_face_id)->render();
      }
      l_list->xgen_alembic_out_ptr_->write_end();
    }
  }

  return MStatus::kSuccess;
}

void xgen_abc_export::parse_args(const MArgList& in_arg) {
  MStatus status;
  MArgDatabase const arg_data{syntax(), in_arg, &status};
  maya_chick(status);
  MSelectionList list{};
  maya_chick(arg_data.getObjects(list));
  p_i->begin_time_ = arg_data.isFlagSet("-s") ? arg_data.flagArgumentMTime("-s", 0) : MAnimControl::minTime();
  p_i->end_time_   = arg_data.isFlagSet("-e") ? arg_data.flagArgumentMTime("-e", 0) : MAnimControl::maxTime();
  if (arg_data.isFlagSet("-n")) p_i->end_time_ = p_i->begin_time_;

  MItSelectionList it_list{list, MFn::kDagNode, &status};
  maya_chick(status);
  std::vector<MDagPath> dag_path_list{};
  for (; !it_list.isDone(); it_list.next()) {
    MDagPath l_dag_path{};
    status = it_list.getDagPath(l_dag_path);
    maya_chick(status);
    // displayInfo(l_dag_path.fullPathName());
    if (auto l_ptr = XgPalette::palette(get_node_name(l_dag_path)); l_ptr)
      p_i->palette_v.emplace_back(l_ptr, l_dag_path);
  }
}

MStatus xgen_abc_export::undoIt() { return MStatus::kSuccess; }

}  // namespace doodle::maya_plug
