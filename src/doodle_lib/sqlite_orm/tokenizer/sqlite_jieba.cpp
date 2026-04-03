#include "sqlite_jieba.h"

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/platform/win/register_file_type.h>
#include <doodle_lib/sqlite_orm/detail/sqlite_database_impl.h>

#include <boost/scope/scope_exit.hpp>

#include <cppjieba/Jieba.hpp>
#include <sqlite3.h>

using xTokenFn = int (*)(void*, int, const char*, int, int, int);
extern "C" int fts5_simple_xCreate(void* sqlite3, const char** azArg, int nArg, Fts5Tokenizer** ppOut);
extern "C" int fts5_simple_xTokenize(
    Fts5Tokenizer* tokenizer_ptr, void* pCtx, int flags, const char* pText, int nText, const char* pLocale, int nLocale,
    xTokenFn xToken
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
    auto l_curr_path      = register_file_type::program_location() / "dict";
    auto l_dict_path      = (l_curr_path / "jieba.dict.utf8").generic_string();
    auto l_model_path     = (l_curr_path / "hmm_model.utf8").generic_string();
    auto l_user_dict_path = (l_curr_path / "user.dict.utf8").generic_string();
    auto l_idf_path       = (l_curr_path / "idf.utf8").generic_string();
    auto l_stop_word_path = (l_curr_path / "stop_words.utf8").generic_string();

    jieba_ =
        std::make_unique<cppjieba::Jieba>(l_dict_path, l_model_path, l_user_dict_path, l_idf_path, l_stop_word_path);
  }

  std::int32_t tokenize(
      void* pCtx, int flags, const char* pText, int nText, const char* pLocale, int nLocale, xTokenFn xToken
  ) {
    std::string text(pText, nText);
    std::vector<std::string> words;
    jieba_->CutForSearch(text, words, true);  // 使用全模式分词

    for (const auto& word : words) {
      xToken(pCtx, flags, word.c_str(), static_cast<int>(word.size()), 0, 0);
    }
    return SQLITE_OK;
  }
};

void register_jieba_tokenizer(sqlite_database_impl& sqlite_db) {
  // Implementation for registering the Jieba tokenizer
  auto l_fts5_api = sqlite_db.get_fts5_api();
  fts5_tokenizer_v2 tokenizer{2, fts5_simple_xCreate, fts5_simple_xDelete, fts5_simple_xTokenize};
  l_fts5_api->xCreateTokenizer_v2(l_fts5_api, "jieba", nullptr, &tokenizer, nullptr);
}
}  // namespace doodle::tokenizer

int fts5_simple_xCreate(void* sqlite3, const char** azArg, int nArg, Fts5Tokenizer** ppOut) {
  (void)sqlite3;
  auto* p = new doodle::tokenizer::jitba_tokenizer(azArg, nArg);
  *ppOut  = reinterpret_cast<Fts5Tokenizer*>(p);
  return SQLITE_OK;
}

int fts5_simple_xTokenize(
    Fts5Tokenizer* tokenizer_ptr, void* pCtx, int flags, const char* pText, int nText, const char* pLocale, int nLocale,
    xTokenFn xToken
) {
  auto* p = reinterpret_cast<doodle::tokenizer::jitba_tokenizer*>(tokenizer_ptr);
  return p->tokenize(pCtx, flags, pText, nText, pLocale, nLocale, xToken);
}

void fts5_simple_xDelete(Fts5Tokenizer* p) {
  auto* pST = reinterpret_cast<doodle::tokenizer::jitba_tokenizer*>(p);
  delete (pST);
}