//
// Created by TD on 2021/7/29.
//

#include "season.h"
namespace doodle {
season::season()
    : Metadata(),
      database_action<season, MetadataFactory>(this) {
}
}  // namespace doodle
