#pragma once

#include "CoreMinimal.h"

namespace doodle
{
  class init_ue4_project
  {
  public:
    static TArray<UObject *> find_sk();

    UObject *p_world_;
    UObject *p_level_;
    FString p_save_world_path;
    FString p_save_level_path;
    TArray<FAssetData> blueprint_list;

    bool load_all_blueprint();
    bool build_all_blueprint();

    bool create_world(const FString &in_path);
    bool create_level(const FString &in_path);
    bool set_level_info(int32 in_start, int32 in_end);
    bool save();

    bool import_ass_data(const FString &in_path, UObject* Outer);

    void tmp();
  };

} // namespace doodle
