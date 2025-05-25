#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <deque>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace flexi_cfg::details {

// Check if the hash and predicate are transparent.
template <typename Hasher, typename Predicate>
concept Transparent = requires {
  typename Hasher::is_transparent;
  typename Predicate::is_transparent;
};

// Checks if type K is either the actual Key type OR can be used in a transparent manner.
template <typename K, typename Key, typename Hash, typename Pred>
concept IsTransparentKey =
    // Condition 1: K is the same as Key.
    std::same_as<std::decay_t<K>, Key> ||
    // Condition 2: The map is transparent AND K can be used by the hash/predicate
    (Transparent<Hash, Pred> && requires(const Hash& h, const Pred& p, const K& k, const Key& key) {
      { h(k) } -> std::convertible_to<std::size_t>;  // Can we hash K?
      { p(k, key) } -> std::convertible_to<bool>;    // Can we compare K and Key
    });

struct string_hash : public std::hash<std::string_view> {
  using is_transparent = void;

  auto operator()(const std::string& str) const -> size_t {
    return std::hash<std::string_view>::operator()(str);
  }

  auto operator()(const std::string_view& str) const -> size_t {
    return std::hash<std::string_view>::operator()(str);
  }

  auto operator()(const char* str) const -> size_t {
    return std::hash<std::string_view>::operator()(str);
  }
};

template <typename Key, typename T, typename Hash = std::hash<Key>, typename Pred = std::equal_to<>,
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
  ordered_map(InputIterator first, InputIterator last) : map_(first, last) {
    std::transform(first, last, std::back_inserter(keys_),
                   [](const auto& value) { return value.first; });
  }

  // Copy constructor
  ordered_map(const ordered_map&) = default;

  // Move constructor
  ordered_map(ordered_map&&) = default;

  // Constructor (1)
  explicit ordered_map(const Pred& /*comp*/, const Alloc& alloc = Alloc()) : map_(alloc) {}

  // Constructor (2)
  explicit ordered_map(const Alloc& alloc) : map_(alloc) {}

  // Constructor (3)
  template <typename InputIterator>
  ordered_map(InputIterator first, InputIterator last, const Pred& /*comp*/,
              const Alloc& alloc = Alloc())
      : map_(first, last, 0, Hash{}, Pred{}, alloc) {
    std::transform(first, last, std::back_inserter(keys_),
                   [](const auto& value) { return value.first; });
  }

  // Constructor (4)
  ordered_map(std::initializer_list<value_type> init, const Pred& /*comp*/,
              const Alloc& alloc = Alloc())
      : map_(init, 0, Hash{}, Pred{}, alloc) {
    std::transform(std::begin(init), std::end(init), std::back_inserter(keys_),
                   [](const value_type& value) { return value.first; });
  }

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
    keys_.clear();
    std::transform(std::begin(l), std::end(l), std::back_inserter(keys_),
                   [](const value_type& value) { return value.first; });
    return *this;
  }

  allocator_type get_allocator() const noexcept { return map_.get_allocator(); }

  [[nodiscard]] bool empty() const noexcept { return map_.empty(); }

  [[nodiscard]] size_type size() const noexcept { return keys_.size(); }

  [[nodiscard]] size_type max_size() const noexcept {
    return std::min(map_.max_size(), keys_.max_size());
  }

  // NOTE: Forward declare const_iterator_ so that iterator_base can be a friend of it.
  class const_iterator_;

  // Custom iterator for the ordered_map object.
  // This iterator should iterate over the keys in the order they were inserted
  template <typename MapType, typename OrderedKeysType>
  class iterator_base {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = typename MapType::value_type;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    iterator_base(MapType* map, OrderedKeysType* keys, size_type index)
        : map_(map), keys_(keys), index_(index) {
      // Check that the offset is within bounds
      if (index_ > keys_->size()) {  // index_ can be keys_->size() for end()
        throw std::out_of_range("Index out of range");
      }
      update_key_from_index();
    }

    iterator_base() = default;

    bool operator==(const iterator_base& other) const { return index_ == other.index_; }
    bool operator!=(const iterator_base& other) const { return index_ != other.index_; }

    friend class const_iterator_;

   protected:
    MapType* map_{nullptr};
    OrderedKeysType* keys_{nullptr};
    size_type index_{0};
    std::optional<typename OrderedKeysType::value_type> key_;

    void update_key_from_index() {
      if (index_ < keys_->size()) {  // Valid for dereferencing
        key_ = (*keys_)[index_];
      } else {
        key_ = std::nullopt;  // For end() or other non-dereferenceable states
      }
    }
  };

  class iterator_ : public iterator_base<Map, OrderedKeys> {
   public:
    using Base = iterator_base<Map, OrderedKeys>;
    using reference = typename Base::reference;
    using pointer = typename Base::pointer;

    friend class const_iterator_;

    iterator_(Map* map, OrderedKeys* keys, size_type index) : Base(map, keys, index) {
      this->update_key_from_index();
    }

    iterator_() = default;

    reference operator*() const {
      // Precondition: Iterator is dereferenceable (key_ has a value).
      // Calling .value() on a std::nullopt will throw std::bad_optional_access,
      // which is acceptable for UB (dereferencing end()).
      return *this->map_->find(this->key_.value());
    }
    pointer operator->() const {
      // Precondition: Iterator is dereferenceable.
      return &(*this->map_->find(this->key_.value()));
    }

    // Prefix increment
    iterator_& operator++() {
      ++this->index_;
      this->update_key_from_index();
      return *this;
    }

    // Postfix increment
    iterator_ operator++(int) {
      auto tmp = *this;
      ++this->index_;
      this->update_key_from_index();
      return tmp;
    }

    iterator_& operator--() {
      --this->index_;
      this->update_key_from_index();
      return *this;
    }

    iterator_ operator--(int) {
      auto tmp = *this;
      --this->index_;
      this->update_key_from_index();
      return tmp;
    }

    template <std::integral I>
    iterator_ operator+(I i) {
      return {this->map_, this->keys_, this->index_ + i};
    }

    template <std::integral I>
    iterator_ operator-(I i) {
      return {this->map_, this->keys_, this->index_ - i};
    }

    template <std::integral I>
    iterator_& operator+=(I i) {
      this->index_ += i;
      this->update_key_from_index();
      return *this;
    }

    template <std::integral I>
    iterator_& operator-=(I i) {
      this->index_ -= i;
      this->update_key_from_index();
      return *this;
    }
  };

  // Custom iterator for the ordered_map object.
  // This iterator should iterate over the keys in the order they were inserted
  class const_iterator_ : public iterator_base<const Map, const OrderedKeys> {
   public:
    using Base = iterator_base<const Map, const OrderedKeys>;
    using reference = typename Map::value_type const&;
    using pointer = typename Map::value_type const*;

    friend class iterator_;
    friend class iterator_base<const Map, const OrderedKeys>;

    const_iterator_(const Map* map, const OrderedKeys* keys, size_type index)
        : Base(map, keys, index) {
      this->update_key_from_index();
    }

    // Converting constructor from iterator_ to const_iterator_
    const_iterator_(const iterator_& other) : Base(other.map_, other.keys_, other.index_) {
      this->update_key_from_index();
    }

    const_iterator_() = default;

    reference operator*() const {
      // Precondition: Iterator is dereferenceable.
      return *this->map_->find(this->key_.value());
    }
    pointer operator->() const {
      // Precondition: Iterator is dereferenceable.
      return &(*this->map_->find(this->key_.value()));
    }

    // Prefix increment
    const_iterator_& operator++() {
      ++this->index_;
      this->update_key_from_index();
      return *this;
    }

    // Postfix increment
    const_iterator_ operator++(int) {
      auto tmp = *this;
      ++this->index_;
      this->update_key_from_index();
      return tmp;
    }

    const_iterator_& operator--() {
      --this->index_;
      this->update_key_from_index();
      return *this;
    }

    const_iterator_ operator--(int) {
      auto tmp = *this;
      --this->index_;
      this->update_key_from_index();
      return tmp;
    }

    template <std::integral I>
    const_iterator_ operator+(I i) {
      return {this->map_, this->keys_, this->index_ + i};
    }

    template <std::integral I>
    const_iterator_ operator-(I i) {
      return {this->map_, this->keys_, this->index_ - i};
    }

    template <std::integral I>
    const_iterator_& operator+=(I i) {
      this->index_ += i;
      this->update_key_from_index();
      return *this;
    }

    template <std::integral I>
    const_iterator_& operator-=(I i) {
      this->index_ -= i;
      this->update_key_from_index();
      return *this;
    }
  };

  // Iterators
  using iterator = iterator_;
  using const_iterator = const_iterator_;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() noexcept { return iter_from_index(0); }
  const_iterator begin() const noexcept { return iter_from_index(0); }
  const_iterator cbegin() const noexcept { return begin(); }

  iterator end() noexcept { return {&map_, &keys_, keys_.size()}; }
  const_iterator end() const noexcept { return {&map_, &keys_, keys_.size()}; }
  const_iterator cend() const noexcept { return end(); }

  // Reverse iterators
  reverse_iterator rbegin() noexcept { return std::reverse_iterator(end()); }
  const_reverse_iterator rbegin() const noexcept { return std::reverse_iterator(end()); }
  reverse_iterator rend() noexcept { return std::reverse_iterator(begin()); }
  const_reverse_iterator rend() const noexcept { return std::reverse_iterator(begin()); }

  const_reverse_iterator crbegin() const noexcept { return std::reverse_iterator(cend()); }
  const_reverse_iterator crend() const noexcept { return std::reverse_iterator(cbegin()); }

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

  iterator insert(const_iterator /*pos*/, const value_type& value) {
    // Hint 'pos' is ignored for placement.
    return this->insert(value).first;
  }

  template <typename Pair>
  std::enable_if_t<std::is_convertible_v<value_type, Pair>, iterator> insert(const_iterator /*pos*/,
                                                                             Pair&& value) {
    // Hint 'pos' is ignored for placement.
    return this->insert(std::forward<Pair>(value)).first;
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
    for (auto it = first; it != last; ++it) {
      this->insert(*it);  // Call the appropriate insert overload for value_type
    }
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
    map_.insert_or_assign(k, std::forward<M>(obj));
    auto it = iter_from_key(std::move(k));
    return {it, true};
  }

  template <class M>
  iterator insert_or_assign(const_iterator /*hint*/, const Key& k, M&& obj) {
    // Hint 'hint' is ignored for placement.
    return this->insert_or_assign(k, std::forward<M>(obj)).first;
  }

  template <class M>
  iterator insert_or_assign(const_iterator /*hint*/, Key&& k, M&& obj) {
    // Hint 'hint' is ignored for placement.
    return this->insert_or_assign(std::move(k), std::forward<M>(obj)).first;
  }

  template <class... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    // Note: This emplace assumes Args... can be passed to std::make_pair
    // to form a type that is convertible to value_type.
    // A more std::map-conformant emplace would directly construct value_type:
    // value_type temp_element(std::forward<Args>(args)...);
    // const Key& key_ref = temp_element.first;
    // if (map_.contains(key_ref)) {
    //   return {find(key_ref), false};
    // }
    // keys_.emplace_back(key_ref);
    // map_.emplace(std::move(temp_element)); // or map_.insert(std::move(temp_element));
    // return {find(key_ref), true};
    // ---
    // Applying minimal fix to the existing structure:
    auto value = std::make_pair(std::forward<Args>(args)...);
    if (map_.contains(value.first)) {
      return {find(value.first), false};
    }
    Key key_copy = value.first;  // Cache key before value (and value.first) is moved.
    keys_.emplace_back(key_copy);
    map_.emplace(std::move(value));
    return {find(key_copy), true};
  }

  template <class... Args>
  iterator emplace_hint(const_iterator /*hint*/, Args&&... args) {
    // Args... are used to construct value_type (std::pair<const Key, T>)
    value_type val(std::forward<Args>(args)...);

    auto map_find_it = map_.find(val.first);
    if (map_find_it != map_.end()) {
      return find(val.first);  // Key already exists
    }

    Key key_for_keys = val.first;
    keys_.push_back(key_for_keys);
    map_.insert(std::move(val));  // Use insert to move the constructed val
    return find(key_for_keys);    // Return iterator to newly inserted element
  }

  template <class... Args>
  std::pair<iterator, bool> try_emplace(const Key& k, Args&&... args) {
    auto it = map_.find(k);
    if (it != map_.end()) {
      return {find(k), false};
    }
    keys_.push_back(k);
    map_.try_emplace(k, std::forward<Args>(args)...);
    return {find(k), true};
  }

  template <class... Args>
  std::pair<iterator, bool> try_emplace(Key&& k, Args&&... args) {
    // Find using k before it's moved.
    auto map_iter = map_.find(k);
    if (map_iter != map_.end()) {
      // Key exists, use the key from the map to get our iterator.
      // k is not moved in this path.
      return {find(map_iter->first), false};
    }

    // Key does not exist. We need to move k into map_ and add its value to keys_.
    // Create a copy of k for keys_ and for find, then move k.
    Key key_val = k;
    keys_.push_back(key_val);
    map_.try_emplace(std::move(k), std::forward<Args>(args)...);
    return {find(key_val), true};
  }

  // Behavior is undefined if pos is not a valid dereferenceable iterator.
  iterator erase(iterator pos) {
    // Assuming pos is valid and dereferenceable as per typical map::erase contracts.
    // If pos == end(), behavior is undefined by C++ standard.
    const auto index = std::distance(begin(), pos);
    // The key must be obtained before keys_ is modified if index is used on keys_ later.
    // pos->first is safe here because pos is assumed dereferenceable.
    auto map_iter_to_erase = map_.find(pos->first);
    if (map_iter_to_erase != map_.end()) {
      map_.erase(map_iter_to_erase);
    }
    // Erase from keys_ vector by index.
    if (index < static_cast<decltype(index)>(keys_.size())) {
      keys_.erase(std::next(std::begin(keys_), index));
    }
    return iter_from_index(index);
  }

  // Behavior is undefined if pos is not a valid dereferenceable iterator.
  iterator erase(const_iterator pos) {
    // Assuming pos is valid and dereferenceable.
    const auto index = std::distance(cbegin(), pos);
    // assert(index >= 0 && index < static_cast<decltype(index)>(keys_.size()) && "Index out of
    // bounds for erase");

    auto map_iter_to_erase = map_.find(pos->first);
    if (map_iter_to_erase != map_.end()) {
      map_.erase(map_iter_to_erase);
    }
    if (index < static_cast<decltype(index)>(keys_.size())) {
      keys_.erase(keys_.begin() + index);
    }
    return iter_from_index(index);
  }

  iterator erase(const_iterator first, const_iterator last) {
    if (first == last) {
      // Need to return a non-const iterator.
      // first.index_ is the index of the element that would be pointed to.
      return iter_from_index(first.index_);
    }

    size_type start_idx = first.index_;
    size_type end_idx =
        last.index_;  // This is one past the last element to erase in terms of original indexing.

    // It's generally safer to collect keys and then erase from map,
    // then erase from vector.
    std::vector<Key> keys_to_remove_from_map;
    keys_to_remove_from_map.reserve(end_idx - start_idx);
    for (size_type i = start_idx; i < end_idx; ++i) {
      // Check if the key_ optional in the iterator has a value.
      // However, first and last are const_iterators, their internal key_ might not be directly
      // accessible or relevant if they are end iterators. We should rely on keys_[i] as the source
      // of truth for keys.
      if (i < keys_.size()) {  // Ensure index is valid for keys_
        keys_to_remove_from_map.push_back(keys_[i]);
      }
    }

    for (const auto& key : keys_to_remove_from_map) {
      map_.erase(key);
    }

    // Erase from keys_ vector. This invalidates iterators at or after start_idx.
    if (start_idx < keys_.size()) {  // Ensure start_idx is valid
      keys_.erase(keys_.begin() + start_idx, keys_.begin() + std::min(end_idx, keys_.size()));
    }

    // The new element at start_idx is the one that followed the erased range.
    return iter_from_index(start_idx);
  }

  template <typename K>
  size_type erase(const K& key) {
    // Use the map's find to check for existence efficiently
    auto map_it = map_.find(key);
    if (map_it == map_.end()) {
      return 0;  // Not found
    }

    // Key exists. Get a copy of the key before erasing from the map.
    Key key_to_erase = map_it->first;
    map_.erase(map_it);

    // Now erase from the vector
    auto keys_it = std::find(keys_.begin(), keys_.end(), key_to_erase);
    if (keys_it != keys_.end()) {
      keys_.erase(keys_it);
    }

    return 1;
  }

  void swap(ordered_map& other) noexcept(
      noexcept(std::declval<Map&>().swap(std::declval<Map&>())) &&
      noexcept(std::declval<OrderedKeys&>().swap(std::declval<OrderedKeys&>()))) {
    map_.swap(other.map_);
    keys_.swap(other.keys_);
  }

  node_type extract(const_iterator position) {
    if (position == cend()) {
      return {};
    }
    // Use the iterator's public interface to get the key
    const auto& key_to_extract = position->first;

    auto keys_it = std::find(keys_.begin(), keys_.end(), key_to_extract);
    if (keys_it != keys_.end()) {
      keys_.erase(keys_it);
    }
    return map_.extract(key_to_extract);
  }

  node_type extract(const Key& k) {
    auto it = std::find(keys_.begin(), keys_.end(), k);
    if (it != keys_.end()) {
      keys_.erase(it);
    }
    return map_.extract(k);
  }

#if 1
  template <class H2, class P2>
  void merge(ordered_map<Key, T, H2, P2, Alloc>& source) {
    std::vector<Key> extracted_keys;
    for (auto& v : source) {
      if (!map_.contains(v.first)) {
        extracted_keys.emplace_back(v.first);
        insert(std::move(source.map_.extract(v.first)));
      }
    }

    // Need to remove the keys from the source map that have been extracted
    for (const auto& k : extracted_keys) {
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

  template <class H2, class P2>
  void merge(ordered_map<Key, T, H2, P2, Alloc>&& source) {
    // Iterate through source. For each element, if its key is not in this map,
    // extract the node from source and insert it into this.
    // Since source is an rvalue reference, its elements can be moved from.
    // Ensure keys_ in both this and source are correctly updated.
    if (this == &source) {  // Self-merge is a no-op
      return;
    }

    std::vector<Key> keys_to_remove_from_source;
    // Iterate using source's order. A bit tricky as source.keys_ will be modified.
    // Best to iterate over a copy of source.keys_ or use indices carefully.
    // Or, iterate while source is not empty, always processing its first element.

    // Let's try iterating over source's keys by index before they are removed.
    // This requires source.keys_ not to be modified during this loop in a way that invalidates
    // indices. We will modify source.map_ by extracting nodes, and collect keys to remove from
    // source.keys_ later.

    for (const auto& key_in_source : source.keys_) {  // Iterate over a snapshot of keys if
                                                      // source.keys_ could change due to extract.
      // However, map_.extract() doesn't change other map elements or keys_ vector.
      // What changes source.keys_ is our explicit removal.
      if (!this->map_.contains(key_in_source)) {
        node_type extracted_node = source.map_.extract(key_in_source);
        if (extracted_node) {                       // Ensure node was actually extracted
          this->insert(std::move(extracted_node));  // This handles this->keys_ and this->map_
          keys_to_remove_from_source.push_back(key_in_source);
        }
      }
    }

    // Now, remove the moved keys from source.keys_
    // This needs to be done carefully to avoid iterator invalidation issues if using erase-remove
    // idiom directly. A simpler way for potentially many removals:
    if (!keys_to_remove_from_source.empty()) {
      OrderedKeys new_source_keys;
      new_source_keys.reserve(source.keys_.size() - keys_to_remove_from_source.size());
      // Create a set of keys to remove for faster lookups
      std::unordered_set<Key, Hash, Pred> keys_removed_set(keys_to_remove_from_source.begin(),
                                                           keys_to_remove_from_source.end());
      for (const auto& k : source.keys_) {
        if (keys_removed_set.find(k) == keys_removed_set.end()) {
          new_source_keys.push_back(k);
        }
      }
      source.keys_ = std::move(new_source_keys);
    }
    // Allocator handling: if allocators are different and nodes are moved,
    // the behavior is defined by std::unordered_map::insert(node_type&&).
    // If allocators don't match, it might copy/move construct new elements instead of just linking
    // the node. This is standard behavior.
  }

  template <typename K>
  T& at(const K& key) {
    return map_.at(key);
  }

  template <typename K>
  const T& at(const K& key) const {
    return map_.at(key);
  }

  T& operator[](const Key& key) {
    if (!map_.contains(key)) {
      keys_.push_back(key);
    }
    return map_[key];
  }

  T& operator[](Key&& key) {
    if (!map_.contains(key)) {
      keys_.push_back(key);
    }
    return map_[key];
  }

  template <typename K>
  size_type count(const K& x) const {
    return map_.count(x);
  }

  template <typename K>
  iterator find(const K& key) {
    return iter_from_key(key);
  }

  template <typename K>
  const_iterator find(const K& key) const {
    return iter_from_key(key);
  }

  template <typename K>
  bool contains(const K& x) const {
    return map_.contains(x);
  }

  template <typename K>
  std::pair<iterator, iterator> equal_range(const K& key) {
    iterator it = find(key);
    if (it != end()) {
      iterator next_it = it;
      ++next_it;  // Advance to get the upper bound of the range
      return {it, next_it};
    }
    return {end(), end()};
  }

  template <typename K>
  std::pair<const_iterator, const_iterator> equal_range(const K& key) const {
    const_iterator it = find(key);
    if (it != end()) {
      const_iterator next_it = it;
      ++next_it;  // Advance to get the upper bound of the range
      return {it, next_it};
    }
    return {end(), end()};
  }

  template <typename K>
  iterator lower_bound(const K& key) {
    return find(key);
  }

  template <typename K>
  const_iterator lower_bound(const K& key) const {
    return find(key);
  }

  template <typename K>
  iterator upper_bound(const K& key) {
    iterator it = find(key);
    if (it != end()) {
      ++it;  // The element after the found key
    }
    return it;  // If key not found, find(key) is end(), so returns end(). If found, returns next.
  }

  template <typename K>
  const_iterator upper_bound(const K& key) const {
    const_iterator it = find(key);
    if (it != end()) {
      ++it;  // The element after the found key
    }
    return it;  // If key not found, find(key) is end(), so returns end(). If found, returns next.
  }

  key_equal key_comp() const { return map_.key_eq(); }  // Returns Pred, which is key_equal

  class value_compare {
    friend class ordered_map;

   protected:
    Pred pred_;
    value_compare(Pred p) : pred_(p) {}

   public:
    bool operator()(const value_type& lhs, const value_type& rhs) const {
      return pred_(lhs.first, rhs.first);
    }
  };

  value_compare value_comp() const { return value_compare(key_comp()); }

 protected:
  template <typename K>
  size_t get_index(const K& key) const {
    // 1. Use the map's find first.
    auto map_it = map_.find(key);
    if (map_it == map_.end()) {
      return keys_.size();  // Key not found, return an index that will translate to end().
    }

    // 2. Key was found. Now find its position in the insertion-order vector.
    //    map_it->first is of type `const Key&`, which is what `std::find` needs.
    const auto keys_it = std::find(std::begin(keys_), std::end(keys_), map_it->first);
    const auto idx = std::distance(std::begin(keys_), keys_it);
    assert(idx >= 0 && "Expected element idx to be non-negative");
    return static_cast<size_t>(idx);
  }

  // Helpers for creating iterators
  iterator iter_from_index(size_t index) { return {&map_, &keys_, index}; }
  const_iterator iter_from_index(size_t index) const { return {&map_, &keys_, index}; }

  template <typename K>
  iterator iter_from_key(const K& key) {
    return {&map_, &keys_, get_index(key)};
  }
  template <typename K>
  const_iterator iter_from_key(const K& key) const {
    return {&map_, &keys_, get_index(key)};
  }
};

}  // namespace flexi_cfg::details