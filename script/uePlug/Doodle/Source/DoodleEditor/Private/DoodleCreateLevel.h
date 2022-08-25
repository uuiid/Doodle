#pragma once

#include "CoreMinimal.h"

class USkeletalMesh;
class UGeometryCache;
class UAnimSequence;

namespace doodle {
class init_ue4_project {
 public:
  template <typename TSubClass>
  static TArray<TSubClass *> filter_by_type(const TArray<UObject *> &in_obj) {
    TArray<TSubClass *> l_out{};
    for (auto &&i : in_obj) {
      if (i->GetClass()->IsChildOf<TSubClass>()) {
        l_out.Add(CastChecked<TSubClass>(i));
      }
    }
    return l_out;
  }

  bool obj_add_level(const TArray<UGeometryCache *> in_obj);
  bool obj_add_level(const TArray<UAnimSequence *> in_obj);
  bool camera_fbx_to_level(const FString &in_fbx_path);
  bool has_obj(const UObject *in_obj);

  UObject *p_world_;
  UObject *p_level_;
  FString p_save_world_path;
  FString p_save_level_path;
  uint64 start_frame;
  uint64 end_frame;

  TArray<FAssetData> blueprint_list;

  bool load_all_blueprint();
  bool build_all_blueprint();

  bool create_world(const FString &in_path);
  bool create_level(const FString &in_path);
  bool set_level_info(int32 in_start, int32 in_end);
  bool save();

  bool import_ass_data(const FString &in_path);

  static void tmp();
};

}  // namespace doodle
