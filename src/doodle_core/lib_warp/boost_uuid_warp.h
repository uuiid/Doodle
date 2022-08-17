#pragma once

#define BOOST_UUID_RANDOM_PROVIDER_FORCE_WINCRYPT
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_hash.hpp>
#include <boost/uuid/uuid_io.hpp>
#undef BOOST_UUID_RANDOM_PROVIDER_FORCE_WINCRYPT
#include <boost/lexical_cast.hpp>

#include <fmt/format.h>

namespace fmt {
/**
 * @brief 格式化boost uuid
 *
 * @tparam  ::boost::uuids::uuid
 */
template <>
struct formatter<::boost::uuids::uuid> : formatter<fmt::string_view> {
  template <typename FormatContext>
  auto format(const ::boost::uuids::uuid &in_, FormatContext &ctx) const -> decltype(ctx.out()) {
    return formatter<fmt::string_view>::format(
        ::boost::uuids::to_string(in_),
        ctx);
  }
};
}  // namespace fmt
