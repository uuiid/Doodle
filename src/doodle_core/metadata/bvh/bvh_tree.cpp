//
// Created by TD on 2022/9/23.
//
#include "bvh_tree.h"

#include <boost/spirit.hpp>



namespace doodle::bvh {
class bvh_tree::impl {
 public:
};
bvh_tree::bvh_tree()
    : p_i(std::make_unique<impl>()) {
}
void bvh_tree::parse(const boost::filesystem::ifstream& in_stream) {
}

bvh_tree::~bvh_tree() = default;
}  // namespace doodle::bvh
