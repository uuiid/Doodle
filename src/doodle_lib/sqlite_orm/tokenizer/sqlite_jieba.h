#pragma once
#include <sqlite3.h>
namespace doodle {
struct sqlite_database_impl;
namespace tokenizer {
// 注册结巴分词器
void register_jieba_tokenizer(fts5_api* in_fts5_api);
}  // namespace tokenizer
}  // namespace doodle