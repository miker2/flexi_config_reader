#pragma once

#include <deque>
#include <unordered_map>
#include <utility>
#include <vector>

namespace flexi_cfg::details {

template <typename Key, typename T, typename Hash = std::hash<Key>,
          typename Pred = std::equal_to<Key>,
          typename Alloc = std::allocator<std::pair<const Key, T>>>
class ordered_map {
  using Map = std::unordered_map<Key, T, Hash, Pred, Alloc>;
  using OrderedKeys = std::vector<Key>;

  // std::deque<std::pair<const Key, T>>;

  Map map_;
  OrderedKeys keys_;

 public:
  using key_type = Map::key_type;
  using value_type = Map::value_type;
  using mapped_type = Map::mapped_type;
  using hasher = Map::hasher;
  using key_equal = Map::key_equal;
  using allocator_type = Map::allocator_type;

  // Iterator related typedefs
  using size_type = Map::size_type;
  // using reference = ;
  // using const_reference = ;
  // using pointer = ;
  // using const_pointer = ;
  // using iterator = ;
  // using const_iterator = ;
  // using reverse_iterator = ;
  // using const_reverse_iterator = ;

  ordered_map() = default;

  explicit ordered_map(size_type n) : map_(n), keys_(n){};

  template <typename InputIterator>
  ordered_map(InputIterator first, InputIterator last, size_type n = 0) : map_(first, last, n) {}

  // Copy constructor
  ordered_map(const ordered_map&) = default;
  
  // Move constructor
  ordered_map(ordered_map&&) = default;

  /// TODO: Add other constructors as necessary

  // Copy assignment operator
  ordered_map& operator=(const ordered_map&) = default;

  // Move assignment operator
  ordered_map& operator=(ordered_map&&) = default;

  ordered_map(std::initializer_list<value_type> l) : map_(l) {
    std::transform(std::begin(l), std::end(l), std::back_inserter(keys_),
    [](const value_type& value) { return value.first; });
  }

  // List assignment operator
  ordered_map& operator=(std::initializer_list<value_type> l)
  {
    map_ = l;
    std::transform(std::begin(l), std::end(l), std::back_inserter(keys_),
    [](const value_type& value) { return value.first; });
    return *this;
  }

  const OrderedKeys& keys() const { return keys_; }
  const Map& map() const { return map_; }

 protected:
};

}  // namespace flexi_cfg::details