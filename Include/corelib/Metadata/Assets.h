//
// Created by teXiao on 2021/4/27.
//

#pragma once

#include <corelib/core_global.h>
#include <corelib/Metadata/Metadata.h>


namespace doodle {
class CORE_API Assets : public Metadata{
 public:
  std::string str() const override;
}
}