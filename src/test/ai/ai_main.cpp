//
// Created by TD on 2022/9/23.
//

#include <doodle_core/doodle_core.h>

#include <boost/test/unit_test.hpp>

#include "torch/torch.h"
#include <iostream>
#include <ostream>
#include <torch/csrc/autograd/generated/variable_factories.h>

BOOST_AUTO_TEST_CASE(ai_base) { std::cout << torch::eye(3) << std::endl; }