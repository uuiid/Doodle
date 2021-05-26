//
// Created by TD on 2021/5/26.
//

#include "Util.h"
namespace doodle{
auto getStreamSize(std::istream& in_stream) -> decltype(in_stream.tellg()){
  auto k_tellg = in_stream.tellg();
  in_stream.seekg(std::ios::end);
  auto size =  in_stream.tellg();
  in_stream.seekg(k_tellg);
  return size;
}
auto getStreamSize(std::ostream& in_stream) -> decltype(in_stream.tellg()){
  auto k_tellg = in_stream.tellp();
  in_stream.seekp(std::ios::end);
  auto size =  in_stream.tellp();
  in_stream.seekp(k_tellg);
  return size;
}
}
