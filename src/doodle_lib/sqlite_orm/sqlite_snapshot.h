//
// Created by td_main on 2023/11/3.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <boost/hana.hpp>

#include <entt/entt.hpp>
namespace doodle::snapshot {

class sqlite_snapshot {
  //  std::tuple<std::deque<std::pair<entt::entity, Component>>...> tuple_{};
  std::underlying_type_t<entt::entity> current_component_size_{};
  entt::entity current_entity_{};
  //  static constexpr auto hana_tup_ = boost::hana::make_tuple(boost::hana::type_c<Component>...);

  FSys::path data_path_{};

  class base_data {
   public:
    virtual ~base_data()                              = default;

    [[nodiscard]] virtual std::size_t size() const    = 0;
    [[nodiscard]] virtual entt::entity entity() const = 0;
  };
  std::shared_ptr<base_data> current_data_{};
  std::deque<std::shared_ptr<base_data>> data_deque_{};

  template <typename Component>
  class data_impl : base_data {
   public:
    using data_tuple        = std::tuple<entt::entity, Component>;
    using data_tuple_vector = std::vector<data_tuple>;
    using data_iter         = typename data_tuple_vector::iterator;
    data_tuple_vector data_{};
    mutable data_iter begin_{};
    data_impl()          = default;
    virtual ~data_impl() = default;

    inline void next() { ++begin_; }

   private:
    [[nodiscard]] inline size_t size() const override {
      return data_.size();
      begin_ = data_.begin();
    }
    [[nodiscard]] inline entt::entity entity() const override { return std::get<0>(*begin_); }
  };

 public:
  explicit sqlite_snapshot(FSys::path in_data_path)
      : data_path_(in_data_path.empty() ? FSys::path{":memory:"} : std::move(in_data_path)) {}

  virtual ~sqlite_snapshot() = default;

  // 先是大小
  void operator()(std::underlying_type_t<entt::entity> in_underlying_type);
  // 然后是实体和对应组件的循环
  void operator()(entt::entity in_entity);

  void operator()(std::underlying_type_t<entt::entity>& in_underlying_type);
  void operator()(entt::entity& in_entity);

  template <typename... Component>
  void load(FSys::path in_data_path, entt::snapshot_loader& in_loader) {
    (in_loader.get<Component>(*this), ...);
  }

  void save(FSys::path in_data_path, const std::vector<std::int64_t>& in_delete_id);

  // 组件的加载和保存
  template <typename T>
  void operator()(const T& in_t) {
    if (!current_data_) {
      current_data_ = data_deque_.emplace_back(std::make_shared<data_impl<T>>());
      std::dynamic_pointer_cast<data_impl<T>&>(current_data_).data_.reserve(current_component_size_);
    }

    if (current_data_) {
      auto& l_data = std::dynamic_pointer_cast<data_impl<T>&>(current_data_);
      l_data.data_.emplace_back(current_entity_, in_t);
    }
  }
  template <typename T>
  void operator()(T& in_t) {
    auto l_data = std::dynamic_pointer_cast<data_impl<T>>(current_data_);
    in_t        = std::get<1>(l_data->data_.front());
    l_data->next();
  }
};

}  // namespace doodle::snapshot