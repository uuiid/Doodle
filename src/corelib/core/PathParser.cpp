#include <corelib/core/PathParser.h>
#include <corelib/Exception/Exception.h>
#include <corelib/core/CoreData.h>

#include <rttr/type>
namespace doodle::pathParser {
PathParser::PathParser()
    : p_id(),
      p_class(),
      p_prefix(),
      p_suffix(),
      p_regex(),
      p_child_parser(){

      };

const int64_t& PathParser::ID() const noexcept {
  return p_id;
}

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
  auto k_met_set_info = k_type.get_method("setInfo", {rttr::type::get<std::string>()});
  if (!k_met_set_info) throw rttr_error(p_class);

  auto k_met_Root = k_type.get_property("p_roots");
  if (!k_met_Root) throw rttr_error(p_class);
  //创建结果
  std::vector<std::reference_wrapper<rttr::variant>> list{};

  for (auto it : fileSys::directory_iterator(k_root)) {
    std::smatch k_match{};
    auto k_str = it.path().generic_string();
    k_str      = std::regex_replace(k_str, std::regex{k_root.generic_string()}, "");
    while (std::regex_search(k_str, k_match, p_regex)) {
      auto info = k_type.create();
      if (!info) throw rttr_error(p_class);

      auto k_reply = k_met_set_info.invoke(info, k_match.str());
      if (!k_reply) throw rttr_method_invoke_class(p_class);

      //转换为根后 添加根目录
      auto& k_ = info.get_value<CoreData>();

      auto k_path = it.path();
      if (!p_suffix.empty())
        k_path /= p_prefix;

      k_met_Root.set_value(info, std::vector<std::shared_ptr<fileSys::path>>{std::make_shared<fileSys::path>(k_path)});

      //推入结果
      list.emplace_back(std::ref(info));
    }
  }
  return list;
}

}  // namespace doodle::pathParser