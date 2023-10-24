//
// Created by td_main on 2023/10/16.
//

#pragma once
#include <doodle_core/core/file_sys.h>

#include <maya_plug/data/dagpath_cmp.h>

#include <fbxsdk.h>
#include <maya/MDagPath.h>
#include <maya/MPlug.h>
#include <maya/MTime.h>
#include <treehh/tree.hh>
#include <vector>

namespace doodle::maya_plug {
namespace fbx_write_ns {
struct fbx_node;
using fbx_node_ptr = std::shared_ptr<fbx_node>;
using fbx_tree_t   = tree<fbx_node_ptr>;
// 额外数据
struct fbx_extra_data {
  fbx_tree_t* tree_{};
  std::map<std::string, fbxsdk::FbxSurfaceLambert*>* material_map_{};
  std::map<MDagPath, MTransformationMatrix, details::cmp_dag>* bind_post{};
  fbx_extra_data() = default;
};

struct fbx_node {
  MDagPath dag_path{};
  FbxNode* node{};
  std::once_flag flag_{};
  fbx_extra_data extra_data_{};

  fbx_node() = default;
  explicit fbx_node(const MDagPath& in_dag_path, FbxNode* in_node) : dag_path(in_dag_path), node(in_node) {}

  void build_node();

  virtual void build_animation(const MTime& in_time) = 0;

  void build_node_transform(MDagPath in_path) const;
  void set_node_transform_matrix(const MTransformationMatrix& in_matrix) const;

  static FbxTime::EMode maya_to_fbx_time(MTime::Unit in_value);

 protected:
  virtual void build_data() = 0;
};

struct fbx_node_transform : public fbx_node {
  fbx_node_transform() = default;
  explicit fbx_node_transform(const MDagPath& in_dag_path, FbxNode* in_node) : fbx_node(in_dag_path, in_node) {}
  void build_data() override;
  void build_animation(const MTime& in_time) override;
};

struct fbx_node_cam : public fbx_node_transform {
  fbx_node_cam() = default;
  explicit fbx_node_cam(const MDagPath& in_dag_path, FbxNode* in_node) : fbx_node_transform(in_dag_path, in_node) {}
  void build_data() override;
  void build_animation(const MTime& in_time) override;
};

struct fbx_node_mesh : public fbx_node_transform {
  fbxsdk::FbxMesh* mesh{};
  std::vector<std::pair<MPlug, FbxBlendShapeChannel*>> blend_shape_channel_{};
  fbx_node_mesh() = default;
  explicit fbx_node_mesh(const MDagPath& in_dag_path, FbxNode* in_node)
      : fbx_node_transform(in_dag_path, in_node), mesh{}, blend_shape_channel_{} {}

  void build_bind_post();
  void build_data() override;
  void build_animation(const MTime& in_time) override;

  void build_mesh();
  void build_skin();
  void build_blend_shape();

  [[nodiscard]] MObject get_skin_custer() const;
  [[nodiscard]] std::vector<MDagPath> find_joint(const MObject& in_msk) const;
  [[nodiscard]] std::vector<MObject> find_blend_shape() const;

  [[nodiscard]] MObject get_bind_post() const;
};

struct fbx_node_joint : public fbx_node_transform {
  fbx_node_joint() = default;
  explicit fbx_node_joint(const MDagPath& in_dag_path, FbxNode* in_node) : fbx_node_transform(in_dag_path, in_node) {}
  void build_data() override;
};

}  // namespace fbx_write_ns
class fbx_write {
  std::shared_ptr<fbxsdk::FbxManager> manager_;

  fbxsdk::FbxScene* scene_;
  FSys::path path_;
  using fbx_node_t           = fbx_write_ns::fbx_node;
  using fbx_node_mesh_t      = fbx_write_ns::fbx_node_mesh;
  using fbx_node_transform_t = fbx_write_ns::fbx_node_transform;
  using fbx_node_joint_t     = fbx_write_ns::fbx_node_joint;
  using fbx_node_ptr         = fbx_write_ns::fbx_node_ptr;
  using fbx_tree_t           = fbx_write_ns::fbx_tree_t;

  fbx_tree_t tree_{};                                                 // 用于存储节点的树
  std::map<std::string, fbxsdk::FbxSurfaceLambert*> material_map_{};  // 用于存储材质的map
  std::map<MDagPath, fbx_node_ptr, details::cmp_dag> node_map_{};     // 用于存储节点的map
  std::vector<MDagPath> joints_{};
  std::map<MDagPath, MTransformationMatrix, details::cmp_dag> bind_post_{};
  void write_end();
  void init();
  void build_tree(const std::vector<MDagPath>& in_vector);
  void build_data();
  void build_animation(const MTime& in_time);

 public:
  fbx_write();
  ~fbx_write();

  void write(
      const std::vector<MDagPath>& in_vector, const MTime& in_begin, const MTime& in_end, const FSys::path& in_path
  );
  void write(const MSelectionList& in_vector, const MTime& in_begin, const MTime& in_end, const FSys::path& in_path);

  void write(MDagPath in_cam_path, const MTime& in_begin, const MTime& in_end, const FSys::path& in_path);
};

}  // namespace doodle::maya_plug
