#include "importance.h"

namespace doodle {
importance::importance()
    : importance(std::string{}){};

importance::importance(std::string in_cutoff_p)
    : cutoff_p(std::move(in_cutoff_p)){};

importance::~importance() = default;

void to_json(nlohmann::json &j, const importance &p) {
  j["cutoff_p"] = p.cutoff_p;
}
void from_json(const nlohmann::json &j, importance &p) {
  j.at("cutoff_p").get_to(p.cutoff_p);
}

}  // namespace doodle
