//
// Created by TD on 25-6-29.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/exception/exception.h>

#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace doodle::ai {

/// @brief 加载 .npy 格式的 float32 数组文件
/// @param path .npy 文件路径
/// @return 形状向量和数据向量
inline std::pair<std::vector<std::int64_t>, std::vector<float>> load_npy(const FSys::path& path) {
  std::ifstream file(path, std::ios::binary);
  DOODLE_CHICK(file.is_open(), "无法打开 .npy 文件: {}", path.string());

  // 读取魔数 \x93NUMPY
  char magic[6]{};
  file.read(magic, 6);
  constexpr char kMagic[] = {static_cast<char>(0x93), 'N', 'U', 'M', 'P', 'Y'};
  DOODLE_CHICK(
      std::equal(std::begin(magic), std::end(magic), std::begin(kMagic)),
      "无效的 .npy 魔数: {}", path.string()
  );

  // 读取版本号
  std::uint8_t major = 0, minor = 0;
  file.read(reinterpret_cast<char*>(&major), 1);
  file.read(reinterpret_cast<char*>(&minor), 1);
  DOODLE_CHICK(major == 1 || major == 2, "不支持的 .npy 版本: {}.{}", major, minor);

  // 读取头部长度
  std::uint32_t header_len = 0;
  if (major == 1) {
    std::uint16_t len = 0;
    file.read(reinterpret_cast<char*>(&len), 2);
    header_len = len;
  } else {
    file.read(reinterpret_cast<char*>(&header_len), 4);
  }

  // 读取头部文本
  std::string header(header_len, '\0');
  file.read(header.data(), header_len);

  // 解析 shape 字段: 查找 'shape': (d1, d2, ...)
  auto shape_pos = header.find("'shape':");
  DOODLE_CHICK(shape_pos != std::string::npos, "无法在 .npy 头部找到 shape: {}", path.string());

  auto paren_open = header.find('(', shape_pos);
  auto paren_close = header.find(')', paren_open);
  DOODLE_CHICK(
      paren_open != std::string::npos && paren_close != std::string::npos,
      "无法解析 .npy shape: {}", path.string()
  );

  std::vector<std::int64_t> shape;
  std::string shape_str = header.substr(paren_open + 1, paren_close - paren_open - 1);
  std::istringstream ss(shape_str);
  std::string token;
  while (std::getline(ss, token, ',')) {
    // 移除空格
    token.erase(0, token.find_first_not_of(" \t"));
    token.erase(token.find_last_not_of(" \t") + 1);
    if (!token.empty()) {
      shape.push_back(std::stoll(token));
    }
  }
  // 如果 shape 是 (d,) 格式，上面会漏掉最后的 1，补上
  if (shape.empty()) {
    DOODLE_CHICK(false, "无法解析 .npy shape 字符串: '{}'", shape_str);
  }

  // 计算总元素数
  std::int64_t total = 1;
  for (auto s : shape) total *= s;

  // 读取数据（float32）
  std::vector<float> data(static_cast<std::size_t>(total));
  file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(total * sizeof(float)));
  DOODLE_CHICK(
      static_cast<std::size_t>(file.gcount()) == total * sizeof(float),
      ".npy 文件读取数据不完整: {}, 期望 {} 字节, 实际 {} 字节",
      path.string(), total * sizeof(float), file.gcount()
  );

  return {std::move(shape), std::move(data)};
}

}  // namespace doodle::ai
