#pragma once
#include <sqlite3.h>
namespace doodle {
namespace tokenizer {
// 注册结巴分词器
void register_jieba_tokenizer(fts5_api* in_fts5_api);
}  // namespace tokenizer
}  // namespace doodle