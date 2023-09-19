//
// Created by td_main on 2023/9/18.
//
#include <doodle_lib/doodle_lib_all.h>

#include <msgpack.hpp>

using namespace doodle;
struct test_data {
  std::vector<std::uint8_t> data;
  std::string name;
  std::uint32_t id;
  std::chrono::system_clock::time_point time_point;

  void func() { std::cout << "test_data::func" << std::endl; }

  bool operator()(const std::string in_data) const { return in_data == name; }
  MSGPACK_DEFINE_MAP(data, name, id, time_point);
};
struct msgpack_file {
  std::string data;
};
struct fixture {
  static msgpack_file to_msgobj(const test_data& in_data) {
    msgpack::sbuffer buffer{};
    msgpack::packer<msgpack::sbuffer> packer{buffer};
    packer.pack_map(4)
        .pack("data")
        .pack(in_data.data)
        .pack("name")
        .pack(in_data.name)
        .pack("id")
        .pack(in_data.id)
        .pack("time_point")
        .pack(in_data.time_point);
    return msgpack_file{{buffer.data(), buffer.size()}};
  };
  static void reg_entt() {
    entt::meta<test_data>()
        .data<&test_data::data>("data"_hs)
        .data<&test_data::name>("name"_hs)
        .data<&test_data::id>("id"_hs)
        .data<&test_data::time_point>("time_point"_hs)
        .func<&test_data::func>("func"_hs)
        .func<&test_data::operator()>("operator()"_hs)
        .conv<&fixture::to_msgobj>();

    std::cout << "test_data id " << entt::resolve<test_data>().id() << std::endl;
  }
};

// 序列化
void serialize() {
  test_data data;
  data.data       = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  data.name       = "test";
  data.id         = 1;
  data.time_point = std::chrono::system_clock::now();

  msgpack::sbuffer buffer;
  msgpack::packer<msgpack::sbuffer> packer{buffer};

  packer.pack_map(4)
      .pack("data")
      .pack(data.data)
      .pack("name")
      .pack(data.name)
      .pack("id")
      .pack(data.id)
      .pack("time_point")
      .pack(data.time_point);

  std::cout << buffer.size() << std::endl;
  // to msgobj
  msgpack::object_handle handle = msgpack::unpack(buffer.data(), buffer.size());
  msgpack::object obj           = handle.get();
  for (auto&& i : obj.via.map) {
    std::cout << i.key << " : " << i.val << std::endl;
  }
}

// 使用反射序列化
void serialize_reflect() {
  test_data data;
  data.data       = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  data.name       = "test";
  data.id         = 1;
  data.time_point = std::chrono::system_clock::now();

  entt::meta_any any{data};
  if (any.allow_cast<msgpack_file>()) {
    auto l_file                   = any.cast<msgpack_file>();
    msgpack::object_handle handle = msgpack::unpack(l_file.data.data(), l_file.data.size());
    msgpack::object obj           = handle.get();
    for (auto&& i : obj.via.map) {
      std::cout << i.key << " : " << i.val << std::endl;
    }
  }
}

int main(int argc, char* argv[]) {
  fixture::reg_entt();
  serialize();
  serialize_reflect();
  return 0;
}