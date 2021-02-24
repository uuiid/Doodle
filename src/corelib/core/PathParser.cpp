#include <corelib/core/PathParser.h>
#include <rttr/type>
#include <corelib/Exception/Exception.h>
namespace doodle::pathParser {
PathParser::PathParser()
    : p_class(),
      p_prefix(),
      p_suffix(),
      p_regex(){

      };

std::vector<std::reference_wrapper<rttr::variant>> PathParser::parser(const fileSys::path& root) {
  //初始化根路径
  auto k_root = root;
  if (!p_suffix.empty())
    k_root /= p_suffix;
  k_root = k_root.lexically_normal();

  //创建并检查类型
  auto k_type = rttr::type::get_by_name(p_class);
  if (!k_type) throw rttr_not_class{p_class};
  //创建并检查调用的方法
  auto k_met = k_type.get_method("setInfo", {rttr::type::get<std::string>()});
  if (!k_met) throw rttr_error(p_class);
  //创建结果
  std::vector<std::reference_wrapper<rttr::variant>> list{};

  for (auto it : fileSys::directory_iterator(k_root)) {
    std::smatch k_match{};
    auto k_str = it.path().generic_string();
    k_str      = std::regex_replace(k_str, std::regex{k_root.generic_string()}, "");
    while (std::regex_search(k_str, k_match, p_regex)) {
      auto info = k_type.create();
      if (!info) throw rttr_error(p_class);

      auto k_reply = k_met.invoke(info, k_match.str());
      if (!k_reply) throw rttr_method_invoke_class(p_class);

      //推入结果
      list.emplace_back(std::ref(k_reply));
    }
  }
  return list;
}

}  // namespace doodle::pathParser