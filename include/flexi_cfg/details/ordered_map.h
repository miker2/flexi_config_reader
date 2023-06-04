#pragma once

#include <cassert>
#include <deque>
#include <iostream>
#include <stdexcept>
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
  using key_type = typename Map::key_type;
  using value_type = typename Map::value_type;
  using mapped_type = typename Map::mapped_type;
  using hasher = typename Map::hasher;
  using key_equal = typename Map::key_equal;
  using allocator_type = typename Map::allocator_type;

  // Iterator related typedefs
  using size_type = typename Map::size_type;

  ordered_map() = default;

  // explicit ordered_map(size_type n) : map_(n), keys_(n){};

  template <typename InputIterator>
  ordered_map(InputIterator first, InputIterator last) : map_(first, last) {}

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

  [[nodiscard]] bool empty() const noexcept { return map_.empty(); }

  [[nodiscard]] size_type size() const noexcept { return map_.size(); }

  [[nodiscard]] size_type max_size() const noexcept {
    return min(map_.max_size(), keys_.max_size());
  }

  // Custom iterator for the ordered_map object.
  // This iterator should iterate over the keys in the order they were inserted
  template <typename MapType, typename OrderedKeysType>
  class iterator_base
      : public std::iterator<std::forward_iterator_tag, typename MapType::value_type> {
   public:
    iterator_base(MapType& map, OrderedKeysType& keys, size_type index)
        : map_(map), keys_(keys), index_(index) {
      // Check that the offset is within bounds
      if (index_ > keys_.size()) {
        throw std::out_of_range("Index out of range");
      }
      key_ = get_key();
    }

    iterator_base(MapType& map, OrderedKeysType& keys, OrderedKeysType::value_type key)
        : map_(map), keys_(keys), key_(key) {
      auto it = std::find(keys_.begin(), keys_.end(), key_);
      if (it == keys_.end()) {
        throw std::out_of_range("Key not found");
      }
      index_ = std::distance(keys_.begin(), it);
    }

    // Prefix increment
    iterator_base& operator++() {
      ++index_;
      key_ = get_key();
      return *this;
    }

    // Postfix increment
    iterator_base operator++(int) {
      auto tmp = *this;
      ++index_;
      key_ = get_key();
      return tmp;
    }

    iterator_base operator--() {
      --index_;
      key_ = get_key();
      return *this;
    }

    iterator_base operator--(int) {
      auto tmp = *this;
      --index_;
      key_ = get_key();
      return tmp;
    }

    bool operator==(const iterator_base& other) const { return index_ == other.index_; }
    bool operator!=(const iterator_base& other) const { return index_ != other.index_; }

   protected:
    MapType& map_;
    OrderedKeysType& keys_;
    size_type index_;
    OrderedKeysType::value_type key_;

    OrderedKeysType::value_type get_key() const {
      if (index_ < 0 || index_ >= keys_.size()) {
        // If the index is out of range, return the last key
        return {};
      }
      return keys_[index_];
    }
  };

  class iterator_ : public iterator_base<Map, OrderedKeys> {
   public:
    using pointer = value_type*;
    using reference = value_type&;

    iterator_(Map& map, OrderedKeys& keys, size_type index)
        : iterator_base<Map, OrderedKeys>(map, keys, index) {}

    iterator_(Map& map, OrderedKeys& keys, OrderedKeys::value_type key)
        : iterator_base<Map, OrderedKeys>(map, keys, key) {}

    reference operator*() const { return *this->map_.find(this->key_); }

    pointer operator->() const { return &(*this->map_.find(this->key_)); }

   protected:
    friend class const_iterator_;
  };

  // Custom iterator for the ordered_map object.
  // This iterator should iterate over the keys in the order they were inserted
  class const_iterator_ : public iterator_base<const Map, const OrderedKeys> {
   public:
    using const_pointer = const value_type*;
    using const_reference = const value_type&;

    const_iterator_(const Map& map, const OrderedKeys& keys, size_type index)
        : iterator_base<const Map, const OrderedKeys>(map, keys, index) {}

    const_iterator_(const Map& map, const OrderedKeys& keys, OrderedKeys::value_type key)
        : iterator_base<const Map, const OrderedKeys>(map, keys, key) {}

    // Construct a const_iterator from a non-const iterator
    const_iterator_(const iterator_& it)
        : iterator_base<const Map, const OrderedKeys>(it.map_, it.keys_, it.index) {}

    const_reference operator*() const { return *this->map_.find(this->key_); }

    const_pointer operator->() const { return &(*this->map_.find(this->key_)); }
  };

  // Iterators
  using iterator = iterator_;
  using const_iterator = const_iterator_;
  // using reverse_iterator = ;
  // using const_reverse_iterator = ;

  iterator begin() noexcept { return iter_from_index(0); }
  const_iterator begin() const noexcept { return iter_from_index(0); }
  const_iterator cbegin() const noexcept { return begin(); }

  iterator end() noexcept { return {map_, keys_, keys_.size()}; }
  const_iterator end() const noexcept { return {map_, keys_, keys_.size()}; }
  const_iterator cend() const noexcept { return end(); }

  void clear() noexcept {
    map_.clear();
    keys_.clear();
  }

  std::pair<iterator, bool> insert(const value_type& x) {
    if (map_.contains(x.first)) {
      return {find(x.first), false};
    }
    keys_.emplace_back(x.first);
    map_.insert(x);
    return {iter_from_key(x.first), true};
  }

  std::pair<iterator, bool> insert(value_type&& x) {
    if (map_.contains(x.first)) {
      return {find(x.first), false};
    }
    keys_.emplace_back(x.first);
    map_.insert(std::move(x));
    return {iter_from_key(x.first), true};
  }

  template <typename Pair>
  std::enable_if_t<std::is_convertible_v<value_type, Pair>, std::pair<iterator, bool>> insert(
      Pair&& x) {
    if (map_.contains(x.first)) {
      return {find(x.first), false};
    }
    keys_.emplace_back(x.first);
    map_.emplace(std::forward<Pair>(x));
    return {iter_from_key(x.first), true};
  }

  template <class M>
  std::pair<iterator, bool> insert_or_assign(const Key& key, M&& obj) {
    if (map_.contains(key)) {
      map_[key] = std::forward<M>(obj);
      return {find(key), false};
    }
    keys_.emplace_back(key);
    map_.insert_or_assign(key, std::forward<M>(obj));
    return {iter_from_key(key), true};
  }

  template <class M>
  std::pair<iterator, bool> insert_or_assign(Key&& k, M&& obj) {
    if (map_.contains(k)) {
      map_[k] = std::forward<M>(obj);
      return {find(k), false};
    }
    keys_.emplace_back(k);
    auto it = iter_from_key(k);
    map_.insert_or_assign(std::move(k), std::forward<M>(obj));
    return {it, true};
  }

  template <class M>
  iterator insert_or_assign(const_iterator hint, const Key& k, M&& obj) {
    assert(false && "Not implemented yet");
  }

  template <class M>
  iterator insert_or_assign(const_iterator hint, Key&& k, M&& obj) {
    assert(false && "Not implemented yet");
  }

  template <class... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    auto value = std::make_pair(std::forward<Args>(args)...);
    if (map_.contains(value.first)) {
      return {find(value.first), false};
    }
    keys_.emplace_back(value.first);
    auto it = iter_from_key(value.first);
    map_.emplace(std::move(value));

    return {it, true};
  }

  // TODO: Implement emplace_hint (if necessary, nothing currently uses it)

  // TODO: Implement try_emplace (if necessary, nothing currently uses it)

  // TODO: Determine if these erase methods are doing the right thing if 'pos' is not valid.
  iterator erase(iterator pos) {
    const auto index = std::distance(begin(), pos);
    auto it = map_.find(pos->first);
    keys_.erase(keys_.begin() + index);
    map_.erase(it);
    return iter_from_index(index);
  }

  iterator erase(const_iterator pos) {
    const auto index = std::distance(cbegin(), pos);
    assert(index >= 0 && index < keys_.size() && "Index out of bounds");
    auto it = map_.find(pos->first);
    keys_.erase(keys_.begin() + index);
    map_.erase(it);
    return iter_from_index(index);
  }

  // TODO: Implement this erase - which handles a range of iterators
  // iterator erase(const_iterator first, const_iterator last);

  size_type erase(const Key& key) {
    const auto& it = find(key);
    if (it == end()) {
      return 0;
    }
    erase(it);
    return 1;
  }

  // TODO: Implement swap

  // TODO: Implement extract

  // TODO: IMPLEMENT MERGE
  template <class H2, class P2>
  void merge(ordered_map<Key, T, H2, P2, Alloc>& source);

  template <class H2, class P2>
  void merge(ordered_map<Key, T, H2, P2, Alloc>&& source);

  iterator find(const Key& key) { return {map_, keys_, index_helper(key)}; }

  const_iterator find(const Key& key) const { return {map_, keys_, index_helper(key)}; }

  bool contains(const Key& key) const { return map_.contains(key); }

  const OrderedKeys& keys() const { return keys_; }
  const Map& map() const { return map_; }

 protected:
  size_t index_helper(const Key& key) const {
    const auto key_it = std::find(std::begin(keys_), std::end(keys_), key);
    const auto idx = std::distance(std::begin(keys_), key_it);
    assert(idx >= 0 && "Expected element idx to be non-negative");
    return static_cast<size_t>(idx);
  }

  // Helpers for creating iterators
  iterator iter_from_index(size_t index) { return {map_, keys_, index}; }
  const_iterator iter_from_index(size_t index) const { return {map_, keys_, index}; }
  iterator iter_from_key(const Key& key) { return {map_, keys_, key}; }
  const_iterator iter_from_key(const Key& key) const { return {map_, keys_, key}; }
};

}  // namespace flexi_cfg::details