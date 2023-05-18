#pragma once
#include <Alembic/Abc/All.h>
#include <Alembic/Abc/OObject.h>
#include <cstdint>
#include <maya/MApiNamespace.h>
namespace doodle::alembic {
class transform_writer {
 private:
 public:
  explicit transform_writer(
      const Alembic::Abc::OObject& in_root_obj, const MDagPath& in_dag_path, std::int32_t in_time_index
  );
};
}  // namespace doodle::alembic