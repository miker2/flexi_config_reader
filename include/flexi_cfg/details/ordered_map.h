#pragma once

#include <algorithm>
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
  using node_type = typename Map::node_type;

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
  class iterator_base {
    friend class const_iterator_;

   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename MapType::value_type;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    iterator_base(MapType* map, OrderedKeysType* keys, size_type index)
        : map_(map), keys_(keys), index_(index), key_(get_key()) {
      // Check that the offset is within bounds
      if (index_ > keys_->size()) {
        throw std::out_of_range("Index out of range");
      }
    }

    iterator_base() = default;

    bool operator==(const iterator_base& other) const { return index_ == other.index_; }
    bool operator!=(const iterator_base& other) const { return index_ != other.index_; }

   protected:
    MapType* map_{nullptr};
    OrderedKeysType* keys_{nullptr};
    size_type index_{0};
    typename OrderedKeysType::value_type key_;

    typename OrderedKeysType::value_type get_key() const {
      if (index_ < 0 || index_ >= keys_->size()) {
        // If the index is out of range, return the last key
        return {};
      }
      return (*keys_)[index_];
    }
  };

  class iterator_ : public iterator_base<Map, OrderedKeys> {
    friend class const_iterator_;

   public:
    using Base = iterator_base<Map, OrderedKeys>;
    using reference = typename Base::reference;
    using pointer = typename Base::pointer;

    iterator_(Map* map, OrderedKeys* keys, size_type index)
        : Base(map, keys, index) {}

    iterator_() = default;

    reference operator*() const { return *this->map_->find(this->key_); }
    pointer operator->() const { return &(*this->map_->find(this->key_)); }

    // Prefix increment
    iterator_& operator++() {
      ++this->index_;
      this->key_ = this->get_key();
      return *this;
    }

    // Postfix increment
    iterator_ operator++(int) {
      auto tmp = *this;
      ++this->index_;
      this->key_ = this->get_key();
      return tmp;
    }

    iterator_ operator--() {
      --this->index_;
      this->key_ = this->get_key();
      return *this;
    }

    iterator_ operator--(int) {
      auto tmp = *this;
      --this->index_;
      this->key_ = this->get_key();
      return tmp;
    }
  };

  // Custom iterator for the ordered_map object.
  // This iterator should iterate over the keys in the order they were inserted
  class const_iterator_ : public iterator_base<const Map, const OrderedKeys> {
   public:
    using Base = iterator_base<const Map, const OrderedKeys>;
    using reference = typename Map::value_type const&;
    using pointer = typename Map::value_type const*;

    const_iterator_(const Map* map, const OrderedKeys* keys, size_type index)
        : Base(map, keys, index) {}

    const_iterator_() = default;

    reference operator*() const { return *this->map_->find(this->key_); }
    pointer operator->() const { return &(*this->map_->find(this->key_)); }

    // Prefix increment
    const_iterator_& operator++() {
      ++this->index_;
      this->key_ = this->get_key();
      return *this;
    }

    // Postfix increment
    const_iterator_ operator++(int) {
      auto tmp = *this;
      ++this->index_;
      this->key_ = this->get_key();
      return tmp;
    }

    const_iterator_ operator--() {
      --this->index_;
      this->key_ = this->get_key();
      return *this;
    }

    const_iterator_ operator--(int) {
      auto tmp = *this;
      --this->index_;
      this->key_ = this->get_key();
      return tmp;
    }
  };

  // Iterators
  using iterator = iterator_;
  using const_iterator = const_iterator_;
  // using reverse_iterator = ;
  // using const_reverse_iterator = ;

  iterator begin() noexcept { return iter_from_index(0); }
  const_iterator begin() const noexcept { return iter_from_index(0); }
  const_iterator cbegin() const noexcept { return begin(); }

  iterator end() noexcept { return {&map_, &keys_, keys_.size()}; }
  const_iterator end() const noexcept { return {&map_, &keys_, keys_.size()}; }
  const_iterator cend() const noexcept { return end(); }

  struct insert_return_type {
    iterator position;
    bool inserted = false;
    node_type node;
  };

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

  std::pair<iterator, bool> insert(value_type&& x) {
    if (map_.contains(x.first)) {
      return {find(x.first), false};
    }
    keys_.emplace_back(x.first);
    map_.insert(std::move(x));
    return {iter_from_key(x.first), true};
  }

  iterator insert(const iterator pos, const value_type& value) {
    assert(false && "Not implemented yet");
    return {};
  }

  template <typename Pair>
  std::enable_if_t<std::is_convertible_v<value_type, Pair>, iterator> insert(const iterator pos,
                                                                             Pair&& value) {
    assert(false && "Not implemented yet");
    return {};
  }

  insert_return_type insert(node_type&& nh) {
    if (!nh) {
      return {.position = end(), .inserted = false, .node = std::move(nh)};
    }
    const auto key = nh.key();

    auto map_ret = map_.insert(std::move(nh));
    if (!map_ret.inserted) {
      return {.position = find(key), .inserted = false, .node = std::move(map_ret.node)};
    }

    keys_.emplace_back(key);
    return {.position = iter_from_key(key),
            .inserted = map_ret.inserted,
            .node = std::move(map_ret.node)};
  }

  template <class InputIt>
  void insert(InputIt first, InputIt last) {
    assert(false && "Not implemented yet");
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

  // TODO: Figure out how to make the parameter a 'const_iterator'
  node_type extract(const iterator& position) {
    if (position == end()) {
      return {};
    }
    const auto key = position->first;
    const auto it = std::find(keys_.cbegin(), keys_.cend(), key);
    keys_.erase(it);
    return map_.extract(key);
  }

  node_type extract(const Key& k) {
    auto it = std::find(keys_.begin(), keys_.end(), k);
    if (it != keys_.end()) {
      keys_.erase(it);
    }
    return map_.extract(k);
  }

  // TODO: IMPLEMENT MERGE
#if 1
  template <class H2, class P2>
  void merge(ordered_map<Key, T, H2, P2, Alloc>& source) {
    std::vector<Key> extracted_keys;
    for (auto& v : source) {
      std::cout << "Examining key: '" << v.first << "' : " << v.second << std::endl;
      if (!map_.contains(v.first)) {
        std::cout << "  + New key found!" << std::endl;
        extracted_keys.emplace_back(v.first);
        insert(std::move(source.map_.extract(v.first)));
      }
    }

    // Need to remove the keys from the source map that have been extracted
    for (const auto& k : extracted_keys) {
      std::cout << "Removing key: '" << k << "' from source" << std::endl;
      source.keys_.erase(std::find(source.keys_.begin(), source.keys_.end(), k));
    }
  }
#else
  template <class H2, class P2>
  void merge(ordered_map<Key, T, H2, P2, Alloc>& source) {
    std::vector<Key> extracted_keys;
    size_t skip_count{0};
    for (auto it = source.begin(); it != source.end();) {
      // for (auto& v : source) {
      std::cout << "Examining key: '" << it->first << "' : " << it->second << std::endl;
      if (!map_.contains(it->first)) {
        std::cout << "  + New key found!" << std::endl;

        auto extracted = source.extract(it->first);
        std::cout << "  - extracted key. Attempting insert" << std::endl;
        std::cout << "  - empty? " << extracted.empty() << std::endl;
        insert(std::move(extracted));
        std::cout << "  - insert complete" << std::endl;
        /*
        extracted_keys.emplace_back(v.first);
        insert(std::move(source.map_.extract(v.first)));
        */

      } else {
        ++skip_count;
      }
      it = std::next(source.begin(), skip_count);
    }

    /*
    // Need to remove the keys from the source map that have been extracted
    for (const auto& k : extracted_keys) {
      std::cout << "Removing key: '" << k << "' from source" << std::endl;
      source.keys_.erase(std::find(source.keys_.begin(), source.keys_.end(), k));
    }
    */
  }
#endif

  /*
  template <class H2, class P2>
  void merge(ordered_map<Key, T, H2, P2, Alloc>&& source) {
    assert(false && "Not implemented yet");
  }
  */

  T& at(const Key& key) { return map_.at(key); }

  const T& at(const Key& key) const { return map_.at(key); }

  T& operator[](const Key& key) { return map_[key]; }

  T& operator[](Key&& key) { return map_[key]; }

  size_type count(const Key& key) const { return map_.count(key); }

  template <class K>
  size_type count(const K& x) const {
    return map_.count(x);
  }

  iterator find(const Key& key) { return iter_from_key(key); }

  const_iterator find(const Key& key) const { return iter_from_key(key); }

  bool contains(const Key& key) const { return map_.contains(key); }

  template <class K>
  bool contains(const K& x) const {
    return map_.contains(x);
  }

  std::pair<iterator, iterator> equal_range(const Key& key) {
    assert(false && "Not implemented yet");
  }

  std::pair<const_iterator, const_iterator> equal_range(const Key& key) const {
    assert(false && "Not implemented yet");
  }

  template <class K>
  std::pair<iterator, iterator> equal_range(const K& x) {
    assert(false && "Not implemented yet");
  }

  template <class K>
  std::pair<const_iterator, const_iterator> equal_range(const K& x) const {
    assert(false && "Not implemented yet");
  }

#if 0
  // TODO: Remove these accessors to the underlying keys and map
  const OrderedKeys& keys() const { return keys_; }
  const Map& map() const { return map_; }

  OrderedKeys& keys() { return keys_; }
  Map& map() { return map_; }
#endif
 protected:
  size_t get_index(const Key& key) const {
    const auto key_it = std::find(std::begin(keys_), std::end(keys_), key);
    const auto idx = std::distance(std::begin(keys_), key_it);
    assert(idx >= 0 && "Expected element idx to be non-negative");
    return static_cast<size_t>(idx);
  }

  // Helpers for creating iterators
  iterator iter_from_index(size_t index) { return {&map_, &keys_, index}; }
  const_iterator iter_from_index(size_t index) const { return {&map_, &keys_, index}; }
  iterator iter_from_key(const Key& key) { return {&map_, &keys_, get_index(key)}; }
  const_iterator iter_from_key(const Key& key) const { return {&map_, &keys_, get_index(key)}; }
};

}  // namespace flexi_cfg::details