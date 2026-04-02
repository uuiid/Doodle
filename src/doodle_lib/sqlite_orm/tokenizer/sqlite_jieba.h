#pragma once

namespace doodle {
struct sqlite_database_impl;
namespace tokenizer {
// 注册结巴分词器
void register_jieba_tokenizer(sqlite_database_impl& sqlite_db);
}  // namespace tokenizer
}  // namespace doodle