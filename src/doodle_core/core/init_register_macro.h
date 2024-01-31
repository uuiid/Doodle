//
// Created by td_main on 2023/4/10.
//

#pragma once

namespace doodle::details {
template <typename t>
struct registrar_lambda;

}  // namespace doodle::details
#define DOODLE_REGISTER_FRIEND() \
  template <typename T>          \
  friend struct ::doodle::details::registrar_lambda