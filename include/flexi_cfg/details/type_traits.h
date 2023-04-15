#pragma once

#include <memory>

namespace flexi_cfg::details::type_traits {
  // Similar to 'std::is_pointer<T>' this type trait allows for detection of a smart point
// (std::unique_ptr, std::shared_ptr, std::weak_ptr)
template <typename T, typename = void>
struct is_smart_pointer : std::false_type {};

template <typename T>
struct is_smart_pointer<
    T, std::enable_if_t<std::is_same_v<std::decay_t<T>,
                                       std::unique_ptr<typename std::decay_t<T>::element_type,
                                                       typename std::decay_t<T>::deleter_type>>>>
    : std::true_type {};

template <typename T>
struct is_smart_pointer<
    T, typename std::enable_if_t<std::is_same_v<
           std::decay_t<T>, std::shared_ptr<typename std::decay_t<T>::element_type>>>>
    : std::true_type {};

template <typename T>
struct is_smart_pointer<
    T, typename std::enable_if_t<
           std::is_same_v<std::decay_t<T>, std::weak_ptr<typename std::decay_t<T>::element_type>>>>
    : std::true_type {};

template <typename T>
inline constexpr bool is_smart_pointer_v = is_smart_pointer<T>::value;
}