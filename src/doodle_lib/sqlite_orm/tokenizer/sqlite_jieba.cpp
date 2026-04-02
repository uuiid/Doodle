#include "sqlite_jieba.h"

#include "doodle_core/exception/exception.h"

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/scope/scope_exit.hpp>

#include <cppjieba/Jieba.hpp>
#include <sqlite3.h>

using xTokenFn = int (*)(void*, int, const char*, int, int, int);
extern "C" int fts5_simple_xCreate(void* sqlite3, const char** azArg, int nArg, Fts5Tokenizer** ppOut);
extern "C" int fts5_simple_xTokenize(
    Fts5Tokenizer* tokenizer_ptr, void* pCtx, int flags, const char* pText, int nText, xTokenFn xToken
);
extern "C" void fts5_simple_xDelete(Fts5Tokenizer* tokenizer_ptr);

namespace doodle::tokenizer {
class jitba_tokenizer {
 private:
  std::unique_ptr<cppjieba::Jieba> jieba_;

 public:
  explicit jitba_tokenizer(const char** azArg, int nArg) {
    // 这里可以根据 azArg 和 nArg 来配置分词器，例如加载不同的词典等
    // 目前我们不使用这些参数，直接初始化结巴分词器
    auto l_curr_path = FSys::current_path();
    auto l_dict_path = (l_curr_path / "dict").generic_string();

    jieba_ = std::make_unique<cppjieba::Jieba>(l_dict_path, l_dict_path, l_dict_path, l_dict_path, l_dict_path);
  }

  std::int32_t tokenize(void* pCtx, int flags, const char* pText, int nText, xTokenFn xToken) {
    std::string text(pText, nText);
    std::vector<std::string> words;
    jieba_->CutForSearch(text, words, true);  // 使用全模式分词

    for (const auto& word : words) {
      xToken(pCtx, flags, word.c_str(), static_cast<int>(word.size()), 0, 0);
    }
    return SQLITE_OK;
  }
};
/*
** 返回指向数据库连接 db 的 fts5_api 指针。
** 如果发生错误，则返回 NULL 并在数据库句柄中留下错误信息（可通过 sqlite3_errcode()/errmsg() 访问）。
*/
fts5_api* fts5_api_from_db(sqlite3* db) {
  fts5_api* pRet      = 0;
  sqlite3_stmt* pStmt = 0;
  boost::scope::scope_exit stmt_guard([&]() {
    if (pStmt) sqlite3_finalize(pStmt);
  });

  if (SQLITE_OK == sqlite3_prepare(db, "SELECT fts5(?1)", -1, &pStmt, 0)) {
    sqlite3_bind_pointer(pStmt, 1, (void*)&pRet, "fts5_api_ptr", NULL);
    sqlite3_step(pStmt);
  } else {
    // 处理错误，例如记录日志
    auto l_msg = sqlite3_errmsg(db);
    SPDLOG_ERROR("Failed to prepare statement: {}", l_msg);
    throw_exception(doodle_error{l_msg});
  }
  return pRet;
}

void register_jieba_tokenizer(sqlite_database_impl& sqlite_db) {
  // Implementation for registering the Jieba tokenizer
}
}  // namespace doodle::tokenizer

int fts5_simple_xCreate(void* sqlite3, const char** azArg, int nArg, Fts5Tokenizer** ppOut) {
  (void)sqlite3;
  auto* p = new doodle::tokenizer::jitba_tokenizer(azArg, nArg);
  *ppOut  = reinterpret_cast<Fts5Tokenizer*>(p);
  return SQLITE_OK;
}

int fts5_simple_xTokenize(
    Fts5Tokenizer* tokenizer_ptr, void* pCtx, int flags, const char* pText, int nText, xTokenFn xToken
) {
  auto* p = reinterpret_cast<doodle::tokenizer::jitba_tokenizer*>(tokenizer_ptr);
  return p->tokenize(pCtx, flags, pText, nText, xToken);
}

void fts5_simple_xDelete(Fts5Tokenizer* p) {
  auto* pST = reinterpret_cast<doodle::tokenizer::jitba_tokenizer*>(p);
  delete (pST);
}