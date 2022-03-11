#pragma once

#include "CoreMinimal.h"

namespace doodle {
class init_ue4_project {
 public:
  UObject* p_world_;
  UObject* p_level_;
  FString p_save_world_path;
  FString p_save_level_path;

  bool create_world(const FString& in_path, const FString& in_name);
  bool create_level(const FString& in_path, const FString& in_name);
  bool set_level_info(int32 in_start, int32 in_end);
  bool save();


  void tmp();
};

}  // namespace doodle
