//
// Created by TD on 2025/9/3.
//

#include "export_xgen_abc.h"

#include "maya_plug/maya_comm/add_entt.h"
#include <maya_plug/data/maya_file_io.h>

#include <maya/MAnimControl.h>
#include <maya/MArgDatabase.h>
#include <maya/MDistance.h>
#include <maya/MFn.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMesh.h>
#include <maya/MItSelectionList.h>
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

class XgenRender : public XGenRenderAPI::ProceduralCallbacks {
 public:
  xgen_abc_export* p_owner;
  XgenRender(xgen_abc_export* in_owner) : p_owner{in_owner} {}

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
  p_owner->displayInfo(in_geom);
  using namespace XGenRenderAPI;
  auto l_num = in_cache->get(PrimitiveCache::NumMotionSamples);
  p_owner->displayInfo(fmt::format("{} num sp {}", in_geom, l_num).c_str());
}
void XgenRender::log(const char* in_str) {
  p_owner->displayInfo(in_str);
  default_logger_raw()->info(in_str);
}
bool XgenRender::get(EBoolAttribute in_attr) const { return false; }  /// 已经确认之间硬编码为 false
float XgenRender::get(EFloatAttribute in_attr) const { return 0.f; }
const char* XgenRender::get(EStringAttribute) const { return ""; }
const float* XgenRender::get(EFloatArrayAttribute) const { return nullptr; }
unsigned XgenRender::getSize(EFloatArrayAttribute) const { return 0; }
const char* XgenRender::getOverride(const char* in_name) const { return ""; }    /// 确切不返回任何的覆盖属性
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
  FSys::path l_file_path = maya_file_io::get_current_path();
  l_file_path.replace_extension();
  FSys::path l_geom_file_path{"D:/test_files/test_xgen/cache/alembic/cache.abc"};

  auto l_str = fmt::format(
      "-debug 1 -warning 1 -stats 1 {8} {1} -palette {2} -description {3} -patch {4} -frame {5} "
      "-file {6}__{2}.xgen -geom {7} -shutter 0.0  -world {0};0;0;0;0;{0};0;0;0;0;{0};0;0;0;0;1",
      l_unit_conv, l_namespace, in_.palette_ptr->name(), in_des->name(), in_patch->name(),
      l_current_frame.as(MTime::uiUnit()), l_file_path, l_geom_file_path, !l_namespace.empty() ? "-nameSpace" : ""
  );

  return l_str;
}

MStatus xgen_abc_export::redoIt() {
  XgenRender l_callback{this};
  for (auto&& i : p_i->palette_v) {
    for (auto j = 0; j < i.palette_ptr->numDescriptions(); ++j) {
      auto l_des = i.palette_ptr->description(j);
      for (auto&& [l_path_name, l_ptr] : l_des->patches()) {
        if (!l_des || !l_ptr) {
          auto l_des_name = l_des ? l_des->name() : std::string{};
          auto l_str = fmt::format("集合 {} 描述 {} 为空(请刷新xgen预览), 无法解析", i.palette_ptr->name(), l_des_name);
          displayError(conv::to_ms(l_str));
          default_logger_raw()->info(l_str);
          continue;
        }

        auto l_args   = p_i->create_render_args(i, l_des, l_ptr);
        auto l_render = std::shared_ptr<XGenRenderAPI::PatchRenderer>{
            XGenRenderAPI::PatchRenderer::init(&l_callback, l_args.c_str())
        };
        if (!l_render) continue;
        XGenRenderAPI::bbox l_sub_bbox;
        unsigned int l_face_id = -1;
        while (l_render->nextFace(l_sub_bbox, l_face_id)) {
          auto* l_face = XGenRenderAPI::FaceRenderer::init(l_render.get(), l_face_id);
          l_face->render();
        }
      }
    }

    //     auto l_des       = l_ptr->description(i);
    //     auto l_previewer = l_des->activePreviewer();
    //     auto l_generator = l_des->activeGenerator();
    //     auto l_primitive = l_des->activePrimitive();
    //     auto l_path      = l_primitive->cPatch();
    //     auto l_geom      = l_primitive->cGeom();
    //     auto l_str       = fmt::format("previewer {}", l_previewer->totalEmitCount());
    //     displayInfo(l_str.c_str());
    //     // if (l_primitive->typeName() == "SplinePrimitive") {
    //     //   const auto l_primitive_spline = dynamic_cast<XgSplinePrimitive*>(l_primitive);
    //     //   safevector<SgVec3d> l_geom_v{};
    //     //   SgCurveUtil::mkPolyLine(false, l_primitive_spline->getGeom(), l_geom_v);
    //     //   auto l_size = l_geom_v.size();
    //     //   displayInfo(
    //     //       fmt::format(
    //     //           "{} geom num {} path {} , spline geom num {}", l_des->name(), l_geom.size(), l_path->name(),
    //     l_size
    //     //       )
    //     //           .c_str()
    //     //   );
    //     // }
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
