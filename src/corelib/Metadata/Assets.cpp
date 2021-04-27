//
// Created by teXiao on 2021/4/27.
//

#include <corelib/Metadata/Assets.h>
namespace doodle {
Assets::Assets(std::string in_name)
    : p_name(std::move(in_name)) {}

std::string Assets::str() const {
  return std::string();
}

}