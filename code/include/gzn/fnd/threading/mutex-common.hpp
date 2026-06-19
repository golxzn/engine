#pragma once

namespace gzn::fnd {

namespace util {

template<class T>
concept mutex_like = requires(T mutex) {
  { mutex.lock() };
  { mutex.unlock() };
  { mutex.try_lock() } -> std::convertible_to<bool>;
};

} // namespace util

struct dummy_mutex {
  constexpr void lock() noexcept {}

  constexpr auto try_lock() noexcept -> bool { return true; }

  constexpr void unlock() noexcept {}
};

} // namespace gzn::fnd
