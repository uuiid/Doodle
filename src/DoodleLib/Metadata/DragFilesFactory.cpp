//
// Created by TD on 2021/6/17.
//

#include "DragFilesFactory.h"
doodle::DragFilesFactory::DragFilesFactory() {
}
doodle::ActionPtr doodle::DragFilesFactory::get_action() {
  return doodle::ActionPtr();
}
doodle::ActionPtr doodle::DragFilesFactory::operator()() {
  return doodle::ActionPtr();
}
