//
// Created by td_main on 2023/10/16.
//

#pragma once
#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/core/file_sys.h>
#include <doodle_core/logger/logger.h>

#include <maya_plug/data/dagpath_cmp.h>

#include <fbxsdk.h>
#include <maya/MDagPath.h>
#include <maya/MEulerRotation.h>
#include <maya/MObjectArray.h>
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
  logger_ptr logger_{};

  std::map<std::string, FbxSurfaceLambert*>* material_map_{};
  fbx_extra_data() = default;
};

struct fbx_node {
  MDagPath dag_path{};
  FbxNode* node{};
  std::once_flag flag_{};
  fbx_extra_data extra_data_{};

  fbx_node() = default;

  explicit fbx_node(const MDagPath& in_dag_path, FbxNode* in_node) : dag_path(in_dag_path), node(in_node) {
  }

  void build_node();

  virtual void build_animation(const MTime& in_time) = 0;

  void build_node_transform(MDagPath in_path) const;

  static FbxTime::EMode maya_to_fbx_time(MTime::Unit in_value);

protected:
  virtual void build_data() = 0;
};

struct fbx_node_transform : public fbx_node {
  fbx_node_transform() = default;

  explicit fbx_node_transform(const MDagPath& in_dag_path, FbxNode* in_node) : fbx_node(in_dag_path, in_node) {
  }

  void build_data() override;
  void build_animation(const MTime& in_time) override;
};

struct fbx_node_cam : public fbx_node_transform {
  fbx_node_cam() = default;
  FbxCamera *camera_{};
  std::double_t film_aperture_{};

  explicit fbx_node_cam(const MDagPath& in_dag_path, FbxNode* in_node) : fbx_node_transform(in_dag_path, in_node) {
  }

  void build_data() override;
  void build_animation(const MTime& in_time) override;
};

struct fbx_node_mesh : public fbx_node_transform {
  FbxMesh* mesh{};
  std::vector<std::pair<MPlug, FbxBlendShapeChannel*>> blend_shape_channel_{};

  fbx_node_mesh() = default;

  explicit fbx_node_mesh(const MDagPath& in_dag_path, FbxNode* in_node)
    : fbx_node_transform(in_dag_path, in_node), mesh{}, blend_shape_channel_{} {
  }

  virtual void build_data() override;
  virtual void build_animation(const MTime& in_time) override;

  virtual void build_mesh();
  virtual void build_skin();
  virtual void build_blend_shape();

  [[nodiscard]] virtual MObject get_skin_custer() const;
  [[nodiscard]] virtual std::vector<MDagPath> find_joint(const MObject& in_msk) const;
  [[nodiscard]] virtual std::vector<MObject> find_blend_shape() const;

  [[nodiscard]] virtual MObject get_bind_post() const;
};

struct fbx_node_sim_mesh : public fbx_node_mesh {
  fbx_node_sim_mesh() = default;

  explicit fbx_node_sim_mesh(const MDagPath& in_dag_path, FbxNode* in_node) : fbx_node_mesh(in_dag_path, in_node) {
  }

  void build_data() override;
  void build_animation(const MTime& in_time) override;


  void build_skin() override;
  void build_blend_shape() override;
  [[nodiscard]] MObject get_skin_custer() const override;
  [[nodiscard]] std::vector<MDagPath> find_joint(const MObject& in_msk) const override;
  [[nodiscard]] std::vector<MObject> find_blend_shape() const override;
  [[nodiscard]] MObject get_bind_post() const override;
};

struct fbx_node_joint : public fbx_node_transform {
  fbx_node_joint() = default;

  explicit fbx_node_joint(const MDagPath& in_dag_path, FbxNode* in_node) : fbx_node_transform(in_dag_path, in_node) {
  }

  void build_data() override;
};
} // namespace fbx_write_ns
class fbx_write {
  std::shared_ptr<FbxManager> manager_;

  FbxScene* scene_;
  FSys::path path_;
  using fbx_node_t           = fbx_write_ns::fbx_node;
  using fbx_node_mesh_t      = fbx_write_ns::fbx_node_mesh;
  using fbx_node_sim_mesh_t  = fbx_write_ns::fbx_node_sim_mesh;
  using fbx_node_transform_t = fbx_write_ns::fbx_node_transform;
  using fbx_node_joint_t     = fbx_write_ns::fbx_node_joint;
  using fbx_node_ptr         = fbx_write_ns::fbx_node_ptr;
  using fbx_tree_t           = fbx_write_ns::fbx_tree_t;

  fbx_tree_t tree_{}; // 用于存储节点的树
  mutable std::map<std::string, FbxSurfaceLambert*> material_map_{}; // 用于存储材质的map
  std::map<MDagPath, fbx_node_ptr, details::cmp_dag> node_map_{}; // 用于存储节点的map
  std::vector<MDagPath> joints_{};
  MObjectArray bind_pose_array_{};

  struct fbx_logger {
    logger_ptr logger_{};
  };

  logger_ptr logger_{};

  bool export_anim_{true};
  bool ascii_fbx_{false};
  std::pair<MTime, MTime> anim_time_{};

  void init();
  void build_tree(const std::vector<MDagPath>& in_vector, const std::vector<MDagPath>& in_sim_vector);
  void build_data();
  void build_animation(const MTime& in_time);
  MTime find_begin_anim_time();
  static std::vector<MDagPath> select_to_vector(const MSelectionList& in_vector);

public:
  fbx_write();
  ~fbx_write();

  void not_export_anim(bool in_value = true);
  void ascii_fbx(bool in_value = true);
  void set_path(const FSys::path& in_path);
  void set_logger(const logger_ptr& in_logger);

  void write_end();
  // 写出fbx
  inline void write(const std::vector<MDagPath>& in_vector, const MTime& in_begin, const MTime& in_end) {
    write(in_vector, {}, in_begin, in_end);
  }

  inline void write(const MSelectionList& in_vector, const MTime& in_begin, const MTime& in_end) {
    write(select_to_vector(in_vector), in_begin, in_end);
  }

  // 写出fbx
  void write(
    const std::vector<MDagPath>& in_vector, const std::vector<MDagPath>& in_sim_vector, const MTime& in_begin,
    const MTime& in_end
  );

  inline void write(
    const MSelectionList& in_vector, const MSelectionList& in_sim_vector, const MTime& in_begin, const MTime& in_end
  ) {
    write(select_to_vector(in_vector), select_to_vector(in_sim_vector), in_begin, in_end);
  }

  // 写出相机
  void write(MDagPath in_cam_path, const MTime& in_begin, const MTime& in_end, std::double_t in_film_aperture);

  // 只寻找mesh节点
  fbx_write_ns::fbx_node* find_node(const MDagPath& in_path) const;

  std::pair<MTime, MTime> get_anim_time() const;

  FbxSurfaceLambert* find_material(const std::string& in_name) const;

  // 写出相机
  // void write(MDagPath in_cam_path, const MTime& in_begin, const MTime& in_end);
};
} // namespace doodle::maya_plug