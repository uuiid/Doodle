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
    l_double_array.set(j / (l_konts - 1), j);
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
  bool init_{false};

  void open() {
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
  void creare_curve(
      const XGenRenderAPI::vec3* in_point, std::size_t in_size, std::vector<Alembic::Abc::V3f>& out_points,
      std::vector<std::float_t>& out_knots
  ) {
    out_points.reserve(in_size + out_points.size());
    auto l_begin = out_knots.size();
    auto l_end   = in_size + 2 + out_knots.size();
    out_knots.reserve(l_end);
    for (auto j = 0; j < in_size; j++) {
      out_points.emplace_back(in_point[j].x, in_point[j].y, in_point[j].z);
      out_knots.emplace_back(static_cast<std::float_t>(j) / (in_size + 2 - 1));
    }
    out_knots[l_begin + 1] = 0.0;
    out_knots[l_begin + 2] = 0.0;
    out_knots[l_end - 2]   = out_knots[l_end - 3];
    out_knots[l_end - 1]   = out_knots[l_end - 3];
  }

 public:
  explicit xgen_alembic_out(FSys::path in_path, const MTime& in_begin_time, const MTime& in_end_time)
      : begin_time_(in_begin_time), end_time_(in_end_time), out_path_(std::move(in_path)) {
    open();
  };

  void write(XGenRenderAPI::PrimitiveCache* in_cache) {
    if (!init_) {  // 初始化
      init_ = true;
    }
    // 写入动画
    std::vector<std::int32_t> l_vertices{};
    std::vector<Alembic::Abc::V3f> l_points{};
    std::vector<std::float_t> l_widths{};
    std::vector<std::float_t> l_knots{};
    using namespace XGenRenderAPI;
    bool bIsSpline = in_cache->get(PrimitiveCache::PrimIsSpline);
    if (!bIsSpline) return;

    auto l_num_samples = in_cache->get(PrimitiveCache::NumMotionSamples);
    auto l_const_width = in_cache->get(PrimitiveCache::ConstWidth);
    if (l_const_width) l_widths.emplace_back(in_cache->get(PrimitiveCache::ConstantWidth));

    // 只采样一次, 不使用运动模糊, 保留迭代, 后期可加入运动模糊
    for (auto i = 0; i < l_num_samples; i++) {
      auto* l_pos           = in_cache->get(PrimitiveCache::Points, i);
      auto* l_width         = in_cache->get(PrimitiveCache::Widths);
      auto l_width_size     = in_cache->getSize(PrimitiveCache::Widths);

      auto l_num            = in_cache->get(PrimitiveCache::NumVertices, i);
      const auto l_num_size = in_cache->getSize2(PrimitiveCache::NumVertices, i);
      l_points.reserve(in_cache->getSize2(PrimitiveCache::Points, i));
      l_knots.reserve(in_cache->getSize2(PrimitiveCache::Points, i) + l_num_size * 2);
      l_vertices.reserve(l_num_size);

      std::size_t l_index_off{};
      /// 在maya中,
      /// 玛雅表示：{0,0,0,...,N,N,N}
      /// 外部表示：{0,0,0,0,...,N,N,N,N}
      /// 所有需要去除第一个和开头的两个节点
      for (auto z = 0; z < l_num_size; ++z) {
        creare_curve(l_pos + l_index_off + 1, l_num[z] - 2, l_points, l_knots);
        l_index_off += l_num[z];
        l_vertices.emplace_back(l_num[z]);
      }
      if (!l_const_width) {
        l_widths.reserve(l_width_size);
        for (auto z = 0; z < l_width_size; ++z) l_widths.emplace_back(l_width[z]);
      }
      break;
    }
    Alembic::AbcGeom::OCurvesSchema::Sample l_curve_sample{};
    l_curve_sample.setBasis(Alembic::AbcGeom::kBsplineBasis);
    l_curve_sample.setWrap(Alembic::AbcGeom::kNonPeriodic);
    l_curve_sample.setType(Alembic::AbcGeom::kCubic);
    l_curve_sample.setCurvesNumVertices(l_vertices);
    l_curve_sample.setPositions(l_points);
    l_curve_sample.setWidths(
        Alembic::AbcGeom::OFloatGeomParam::Sample{
            l_widths, l_const_width ? Alembic::AbcGeom::kConstantScope : Alembic::AbcGeom::kVertexScope
        }
    );
    l_curve_sample.setKnots(l_knots);
    auto& l_curve = o_curve_ptr_->getSchema();
    l_curve.set(l_curve_sample);
    Alembic::Abc::Box3d l_box{};
    for (auto&& i : l_points) l_box.extendBy(i);
    o_box3d_property_ptr_->set(l_box);
  };
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
  o_alembic_out_->write(in_cache);
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
  std::string create_render_args(const palette_warp& in_, const XgDescription* in_des, const XgPatch* in_patch);
};
xgen_abc_export::xgen_abc_export() : p_i{std::make_unique<impl>()} {}
xgen_abc_export::~xgen_abc_export() = default;
MSyntax xgen_abc_export_syntax() {
  MSyntax syntax;
  syntax.addFlag("-f", "-file_path", MSyntax::kString);
  syntax.addFlag("-s", "-start", MSyntax::kLong);
  syntax.addFlag("-e", "-end", MSyntax::kLong);
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
  std::double_t l_fps{};
  switch (MTime::uiUnit()) {
    case MTime::k24FPS:
      l_fps = 24;
    case MTime::k25FPS:
      l_fps = 25;
    case MTime::k30FPS:
      l_fps = 30;
    default:
      l_fps = 24;
  }
  FSys::path l_file_path = maya_file_io::get_current_path();
  l_file_path.replace_extension();
  FSys::path l_geom_file_path{"D:/test_files/test_xgen/cache/alembic/cache.abc"};

  auto l_str = fmt::format(
      "-debug 1 -warning 1 -stats 1 {8}{1} -palette {2} -description {3} -patch {4} -frame {5} "
      "-file {6}__{2}.xgen -geom {7} -fps {9} -interpolation linear  -motionSamplesLookup 0.0 "
      "-motionSamplesPlacement 0.0 -world {0};0;0;0;0;{0};0;0;0;0;{0};0;0;0;0;1",
      l_unit_conv, l_namespace, in_.palette_ptr->name(), in_des->name(), in_patch->name(),
      l_current_frame.as(MTime::uiUnit()), l_file_path, l_geom_file_path, !l_namespace.empty() ? "-nameSpace " : "",
      l_fps
  );

  return l_str;
}

MStatus xgen_abc_export::redoIt() {
  auto l_begin_time                   = MAnimControl::minTime();
  auto l_end_time                     = MAnimControl::maxTime();
  auto l_abc_file_path_gen            = std::make_shared<reference_file_ns::generate_abc_file_path>();
  l_abc_file_path_gen->begin_end_time = {l_begin_time, l_end_time};
  std::map<XgDescription*, std::shared_ptr<xgen_alembic_out>> l_out_list{};
  for (auto&& i : p_i->palette_v) {
    auto l_namespace = get_name_space(i.palette_dag);
    for (auto j = 0; j < i.palette_ptr->numDescriptions(); ++j) {
      auto l_des = i.palette_ptr->description(j);

      if (!l_des) {
        displayError(
            conv::to_ms(fmt::format("集合 {} 描述 index {} 为空(请刷新xgen预览), 无法解析", i.palette_ptr->name(), j))
        );
        continue;
      }
      l_abc_file_path_gen->add_external_string = l_des->name();
      std::shared_ptr<xgen_alembic_out> l_xgen_alembic_out_ptr{};
      if (l_out_list.contains(l_des))
        l_xgen_alembic_out_ptr = l_out_list.at(l_des);
      else {
        auto l_out_path = (*l_abc_file_path_gen)(l_namespace);
        displayInfo(conv::to_ms(fmt::format("导出路径 {}", l_out_path)));
        l_xgen_alembic_out_ptr =
            l_out_list.emplace(l_des, std::make_shared<xgen_alembic_out>(l_out_path, l_begin_time, l_end_time))
                .first->second;
      }
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
        XGenRenderAPI::PatchRenderer::deleteTempRenderPalettes();
        XgenRender l_callback{this, l_xgen_alembic_out_ptr};
        auto l_args = p_i->create_render_args(i, l_des, l_ptr);
        displayInfo(l_args.c_str());
        auto l_render = std::shared_ptr<XGenRenderAPI::PatchRenderer>{
            XGenRenderAPI::PatchRenderer::init(&l_callback, l_args.c_str())
        };
        if (!l_render) continue;
        XGenRenderAPI::bbox l_sub_bbox;
        unsigned int l_face_id = -1;
        std::vector<std::unique_ptr<XGenRenderAPI::FaceRenderer>> l_face_list{};
        while (l_render->nextFace(l_sub_bbox, l_face_id))
          if (auto&& l_f = l_face_list.emplace_back(XGenRenderAPI::FaceRenderer::init(l_render.get(), l_face_id)))
            l_f->render();
      }
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
  auto begin_time = arg_data.isFlagSet("-s")
                        ? MTime{boost::numeric_cast<std::double_t>(arg_data.flagArgumentInt("-s", 0)), MTime::uiUnit()}
                        : MAnimControl::minTime();
  auto end_time   = arg_data.isFlagSet("-e")
                        ? MTime{boost::numeric_cast<std::double_t>(arg_data.flagArgumentInt("-e", 0)), MTime::uiUnit()}
                        : MAnimControl::maxTime();
  auto file_name  = arg_data.isFlagSet("-f") ? FSys::path{conv::to_s(arg_data.flagArgumentString("-f", 0))}
                                             : FSys::get_cache_path() / "default.abc";

  default_logger_raw()->info(
      "export_abc_file::doIt: file_name: {}, begin_time: {}, end_time: {}", file_name, begin_time, end_time
  );

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
