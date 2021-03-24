#include <corelib/core/PathParser.h>
#include <corelib/Exception/Exception.h>
#include <corelib/core/CoreData.h>
#include <queue>
namespace doodle::pathParser {
PathParser::PathParser()
    : p_id(-1),
      p_class(),
      p_prefix(),
      p_regex(),
      p_child_parser(){

      };

PathParser::PathParser(std::string cls, std::string regex)
    : p_id(-1),
      p_class(std::move(cls)),
      p_prefix(),
      p_regex(std::regex{std::move(regex), std::regex_constants::icase}),
      p_child_parser(){

      };

const int64_t& PathParser::ID() const noexcept {
  return p_id;
}

std::vector<std::shared_ptr<CoreData>> PathParser::parser(const fileSys::path& root) {
  //初始化根路径
  auto k_root = root.lexically_normal();

  //创建结果
  std::vector<std::shared_ptr<CoreData>> list{};

  auto k_root_regex = std::regex{k_root.generic_string() + "/"};
  for (auto it : fileSys::directory_iterator(k_root)) {
    std::smatch k_match{};
    auto k_str = it.path().generic_string();
    k_str      = std::regex_replace(k_str, k_root_regex, "");

    while (std::regex_search(k_str, k_match, p_regex)) {
      auto k_path = it.path();

      auto k_in = getClassInstance().get();
      // k_in->setInfo(k_str);
      // k_in->setRoots(std::make_shared<fileSys::path>(k_path));

      //推入结果
      list.push_back(k_in);
      k_str = k_match.suffix();
    }
  }
  return list;
}

}  // namespace doodle::pathParser