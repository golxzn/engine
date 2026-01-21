#pragma once

#include <array>
#include <concepts>
#include <type_traits>
#include <utility>

#include "gzn/fnd/assert.hpp"

namespace gzn::fnd::util {


template<class T>
concept iterator_type = requires(std::remove_cvref_t<T> it) {
  *it;
  ++it;
};

template<class T>
concept pointer_like = std::is_pointer_v<T> || std::is_null_pointer_v<T>;

template<class T>
concept has_method_to_address = requires(T const x) { x.to_address(); };

template<class T>
concept has_pointer_traits = requires(T const x) {
  std::pointer_traits<T>::to_address(x);
};

template<class T>
concept has_member_arrow = requires(T const x) { x.operator->(); };

template<class T>
concept addressable_type = pointer_like<T> || has_method_to_address<T> ||
                           has_pointer_traits<T>;

template<addressable_type Iterator>
constexpr auto to_address(Iterator const it) {
  if constexpr (pointer_like<Iterator>) {
    return it;
  } else if constexpr (has_method_to_address<Iterator>) {
    return it.to_address();
  } else {
    return std::pointer_traits<Iterator>::to_address(it);
  }
}


template<class T>
concept has_method_begin = requires(T obj) { obj.begin(); };

template<class T>
concept has_method_end = requires(T const obj) { obj.end(); };

template<class T>
concept has_method_size = requires(T const v) {
  { v.size() } -> std::convertible_to<usize>;
};


template<class Range>
concept has_method_data = requires(Range r) { r.data(); };

template<class T>
concept class_range_type = has_method_data<T> && has_method_size<T>;

template<class T>
concept range_type = class_range_type<T> ||
                     std::is_array_v<std::remove_reference_t<T>>;

constexpr auto data(class_range_type auto &&r) { return r.data(); }

template<class T, std::size_t size>
constexpr auto data(c_array<T, size> const &a) -> T const * {
  return a;
}

template<class T, std::size_t size>
constexpr auto data(c_array<T, size> &a) -> T * {
  return a;
}

template<class T, std::size_t size>
constexpr auto data(c_array<T, size> &&a) -> T * {
  return a;
}

template<class T>
struct is_c_array : std::false_type {
  u64 static constexpr length{ 0 };
};

template<class T, std::size_t N>
struct is_c_array<T[N]> : std::true_type {
  u64 static constexpr length{ static_cast<u64>(N) };
};

template<class T, class SizeType = u64>
inline constexpr auto size(T const &range) noexcept -> SizeType {
  if constexpr (has_method_size<T>) {
    return static_cast<SizeType>(range.size());
  } else if constexpr (is_c_array<T>::value) {
    return static_cast<SizeType>(is_c_array<T>::length);
  } else if constexpr (range_type<T>) {
    return static_cast<SizeType>(
      std::distance(std::begin(range), std::end(range))
    );
  } else {
    gzn_do_static_assert("This type doesn't support util::size");
  }
}

} // namespace gzn::fnd::util
