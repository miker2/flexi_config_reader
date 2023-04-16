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
  ordered_map& operator=(std::initializer_list<value_type> l) {
    map_ = l;
    std::transform(std::begin(l), std::end(l), std::back_inserter(keys_),
                   [](const value_type& value) { return value.first; });
    return *this;
  }

  allocator_type get_allocator() const noexcept { return map_.get_allocator(); }

  [[nodiscard]] bool empty() const noexcept { return map_.empty; }

  [[nodiscard]] size_type size() const noexcept { return map_.size(); }

  [[nodiscard]] size_type max_size() const noexcept {
    return min(map_.max_size(), keys_.max_size());
  }

  // TODO: Implement emplace, emplace_hint (if necessary, nothing currently uses it)

  // Custom iterator for the ordered_map object.
  // This iterator should iterate over the keys in the order they were inserted
  template <typename MapType, typename OrderedKeysType>
  class iterator_base {
   public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Map::value_type;

    iterator_base(MapType& map, OrderedKeysType& keys, size_type index)
        : map_(map), keys_(keys), index_(index) {
      // Check that the offset is within bounds
      if (index_ > keys_.size()) {
        throw std::out_of_range("Index out of range");
      }
    }

    // Prefix increment
    iterator_base& operator++() {
      ++index_;
      return *this;
    }

    // Postfix increment
    iterator_base operator++(int) {
      auto tmp = *this;
      ++index_;
      return tmp;
    }

    bool operator==(const iterator_base& other) const { return index_ == other.index_; }
    bool operator!=(const iterator_base& other) const { return index_ != other.index_; }

   protected:
    MapType& map_;
    OrderedKeysType& keys_;
    size_type index_;
  };

  class iterator_ : public iterator_base<Map, OrderedKeys> {
   public:
    using pointer = value_type*;
    using reference = value_type&;

    iterator_(Map& map, OrderedKeys& keys, size_type index)
        : iterator_base<Map, OrderedKeys>(map, keys, index) {}

    reference operator*() const { return *this->map_.find(this->keys_[this->index_]); }

    pointer operator->() const { return this->map_[this->keys_[this->index_]]; }
  };

  // Custom iterator for the ordered_map object.
  // This iterator should iterate over the keys in the order they were inserted
  class const_iterator_ : public iterator_base<const Map, const OrderedKeys> {
   public:
    using const_pointer = const value_type*;
    using const_reference = const value_type&;

    const_iterator_(const Map& map, const OrderedKeys& keys, size_type index)
        : iterator_base<const Map, const OrderedKeys>(map, keys, index) {}

    const_reference operator*() const { return *this->map_.find(this->keys_[this->index_]); }

    const_pointer operator->() const { return this->map_[this->keys_[this->index_]]; }
  };

  // Iterators
  using iterator = iterator_;
  using const_iterator = const_iterator_;
  // using reverse_iterator = ;
  // using const_reverse_iterator = ;

  iterator begin() noexcept { return {map_, keys_, 0}; }
  const_iterator begin() const noexcept { return {map_, keys_, 0}; }
  const_iterator cbegin() const noexcept { return begin(); }

  iterator end() noexcept { return {map_, keys_, keys_.size()}; }
  const_iterator end() const noexcept { return {map_, keys_, keys_.size()}; }
  const_iterator cend() const noexcept { return end(); }

  const OrderedKeys& keys() const { return keys_; }
  const Map& map() const { return map_; }

 protected:
};

}  // namespace flexi_cfg::details