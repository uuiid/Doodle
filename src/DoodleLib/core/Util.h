//
// Created by TD on 2021/5/26.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle{
/**
 * 这个函数会获得流的大小,
 * @warning 这个会更改流中的指针位置, 虽然最后会更改回去, 但要小心使用
 * @tparam stream 输入或者输出流
 * @param in_stream
 * @return 整个流的大小
 */
auto getStreamSize(std::istream& in_stream) -> decltype(in_stream.tellg());
auto getStreamSize(std::ostream& in_stream) -> decltype(in_stream.tellp());

}
