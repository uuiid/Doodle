//
// Created by TD on 2021/7/29.
//

#pragma once
#include <DoodleLib/Metadata/Metadata.h>
#include <DoodleLib_fwd.h>
namespace doodle {

class DOODLELIB_API season
    : public Metadata,
      public database_action<season, MetadataFactory> {
 public:
  season();

};
}  // namespace doodle
