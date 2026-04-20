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
namespace {
std::size_t utf8_char_len(const unsigned char c) {
  if ((c & 0x80U) == 0U) return 1;
  if ((c & 0xE0U) == 0xC0U) return 2;
  if ((c & 0xF0U) == 0xE0U) return 3;
  if ((c & 0xF8U) == 0xF0U) return 4;
  return 1;
}
}  // namespace

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
    (void)pLocale;
    (void)nLocale;

    std::string text(pText, nText);
    std::vector<cppjieba::Word> words;
    jieba_->CutForSearch(text, words, true);  // 使用搜索模式分词，并保留原文偏移

    auto emit_token = [&](const std::string& in_token, int in_begin, int in_end) -> int {
      return xToken(pCtx, flags, in_token.c_str(), static_cast<int>(in_token.size()), in_begin, in_end);
    };

    for (const auto& word : words) {
      const auto l_begin = static_cast<int>(word.offset);
      const auto l_end   = static_cast<int>(word.offset + word.word.size());
      auto l_result      = emit_token(word.word, l_begin, l_end);
      if (l_result != SQLITE_OK) {
        return l_result;
      }
      // 如果 word 中只含有一个字符，或者全部为 ASCII 字符，则不需要额外处理。
      if (word.word.size() <= 1 ||
          std::all_of(word.word.begin(), word.word.end(), [](unsigned char c) { return c < 128; }))
        continue;

      std::vector<std::size_t> l_offsets;
      l_offsets.reserve(word.word.size());
      for (std::size_t i = 0; i < word.word.size();) {
        l_offsets.emplace_back(i);
        auto l_step = utf8_char_len(static_cast<unsigned char>(word.word[i]));
        if (l_step == 0 || i + l_step > word.word.size()) l_step = 1;
        i += l_step;
      }

      // 额外输出字符级 token，保证“枪”这类单字查询可命中“狙击枪”等词。
      for (std::size_t i = 0; i < l_offsets.size(); ++i) {
        const auto l_char_start = l_offsets[i];
        const auto l_char_end   = (i + 1 < l_offsets.size()) ? l_offsets[i + 1] : word.word.size();
        if (l_char_end <= l_char_start) continue;

        const auto l_char_token = word.word.substr(l_char_start, l_char_end - l_char_start);
        l_result                = emit_token(
            l_char_token, static_cast<int>(word.offset + l_char_start), static_cast<int>(word.offset + l_char_end)
        );
        if (l_result != SQLITE_OK) {
          return l_result;
        }
      }

      // 同时输出 2-gram token，兼顾短词组合检索。
      // for (std::size_t i = 0; i + 1 < l_offsets.size(); ++i) {
      //   const auto l_ngram_start = l_offsets[i];
      //   const auto l_ngram_end = (i + 2 < l_offsets.size()) ? l_offsets[i + 2] : word.word.size();
      //   if (l_ngram_end <= l_ngram_start || (l_ngram_end - l_ngram_start) == word.word.size()) continue;
      //   const auto l_ngram_token = word.word.substr(l_ngram_start, l_ngram_end - l_ngram_start);
      //   l_result = emit_token(
      //       l_ngram_token, static_cast<int>(word.offset + l_ngram_start), static_cast<int>(word.offset + l_ngram_end)
      //   );
      //   if (l_result != SQLITE_OK) {
      //     return l_result;
      //   }
      // }
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