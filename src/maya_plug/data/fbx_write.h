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
struct fbx_node {
  MDagPath dag_path{};
  FbxNode* node{};

  fbx_node() = default;
  explicit fbx_node(const MDagPath& in_dag_path, FbxNode* in_node) : dag_path(in_dag_path), node(in_node) {}

  virtual void build_data(const fbx_tree_t& in_tree)                            = 0;
  virtual void build_animation(const fbx_tree_t& in_tree, const MTime& in_time) = 0;

  void build_node_transform(MDagPath in_path) const;
  static FbxTime::EMode maya_to_fbx_time(MTime::Unit in_value);
};

struct fbx_node_mesh : public fbx_node {
  fbxsdk::FbxMesh* mesh{};
  std::vector<std::pair<MPlug, FbxBlendShapeChannel*>> blend_shape_channel_{};
  fbx_node_mesh() = default;
  explicit fbx_node_mesh(const MDagPath& in_dag_path, FbxNode* in_node)
      : fbx_node(in_dag_path, in_node), mesh{}, blend_shape_channel_{} {}
  void build_data(const fbx_tree_t& in_tree) override;
  void build_animation(const fbx_tree_t& in_tree, const MTime& in_time) override;

  void build_mesh();
  void build_skin(const fbx_tree_t& in_tree);
  void build_blend_shape();

  [[nodiscard]] MObject get_skin_custer() const;
  [[nodiscard]] std::vector<MDagPath> find_joint(const MObject& in_msk) const;
  [[nodiscard]] std::vector<MObject> find_blend_shape() const;
};

struct fbx_node_transform : public fbx_node {
  fbx_node_transform() = default;
  explicit fbx_node_transform(const MDagPath& in_dag_path, FbxNode* in_node) : fbx_node(in_dag_path, in_node) {}
  void build_data(const fbx_tree_t& in_tree) override;
  void build_animation(const fbx_tree_t& in_tree, const MTime& in_time) override;
};

struct fbx_node_joint : public fbx_node_transform {
  fbx_node_joint() = default;
  explicit fbx_node_joint(const MDagPath& in_dag_path, FbxNode* in_node) : fbx_node_transform(in_dag_path, in_node) {}
  void build_data(const fbx_tree_t& in_tree) override;
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

  fbx_tree_t tree_{};                                              // 用于存储节点的树
  std::map<MDagPath, fbx_node_ptr, details::cmp_dag> node_map_{};  // 用于存储节点的map
  std::vector<MDagPath> joints_{};
  void write_end();
  void init();
  void build_tree(const std::vector<MDagPath>& in_vector);
  void build_data();
  void build_animation(const MTime& in_time);

 public:
  fbx_write();
  ~fbx_write();

  void write(
      const std::vector<MDagPath>& in_vector, const MTime& in_begin, const MTime& in_end, const std::string& in_path
  );
};

}  // namespace doodle::maya_plug
