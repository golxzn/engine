#pragma once

#include <exception>
#include <functional>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>

#include "gzn/fnd/assert.hpp"
#include "gzn/fnd/internal/func-details.hpp"

namespace gzn::fnd {

inline constexpr usize FUNC_STORAGE_BYTES_COUNT{
  func_internal::MINIMUM_STORAGE_BYTES_COUNT /*sizeof(void*) * 3*/
};

template<class...>
class copyable_func;

template<class...>
class move_only_func;

namespace func_internal {

template<class...>
struct is_move_only_function_specialization : std::false_type {};

template<class... Ts>
struct is_move_only_function_specialization<move_only_func<Ts...>>
  : std::true_type {};

template<class... Ts>
inline constexpr bool is_move_only_function_specialization_v{
  is_move_only_function_specialization<Ts...>::value
};

template<class...>
struct is_copyable_function_specialization : std::false_type {};

template<class... Ts>
struct is_copyable_function_specialization<copyable_func<Ts...>>
  : std::true_type {};

template<class... Ts>
inline constexpr bool is_copyable_function_specialization_v{
  is_copyable_function_specialization<Ts...>::value
};

template<class>
struct is_in_place_type_t_specialization : std::false_type {};

template<class T>
struct is_in_place_type_t_specialization<std::in_place_type_t<T>>
  : std::true_type {};

template<class T>
inline constexpr bool is_in_place_type_t_specialization_v{
  is_in_place_type_t_specialization<T>::value
};

} // namespace func_internal

template<class Signature>
class move_only_func<Signature> final
  : func_internal::function_call<move_only_func<Signature>, Signature> {

  using traits = func_internal::traits<FUNC_STORAGE_BYTES_COUNT, Signature>;
  using vtable = func_internal::vtable<FUNC_STORAGE_BYTES_COUNT, traits>;
  friend func_internal::function_call<move_only_func, Signature>;

  template<class... T>
  static constexpr bool is_invocable_using{
    traits::template is_invocable_using<T...>
  };

  template<class VT>
  static constexpr bool is_callable_from{
    is_invocable_using<typename traits::template quals<VT>> &&
    is_invocable_using<typename traits::template inv_quals<VT>>
  };

  vtable const            *vptr{ vtable::make_empty() };
  typename vtable::context storage{};
  void                    *allocator{ nullptr };

  template<bool Copyable, class T, class... Args>
  static constexpr inline auto make(Args &&...args) -> decltype(auto) {
    return vtable::template make<Copyable, T>(std::forward<Args>(args)...);
  }

public:
  move_only_func(std::nullptr_t n = nullptr) noexcept {}

  template<class F>
    requires(
      !std::is_same_v<move_only_func, std::remove_cvref_t<F>> &&
      !func_internal::is_in_place_type_t_specialization_v<
        std::remove_cvref_t<F>> &&
      is_callable_from<std::decay_t<F>>
    )
  constexpr explicit move_only_func(
    util::allocator_type auto &alloc,
    F                        &&func
  ) noexcept
    : allocator{ &alloc } {
    using VT       = std::decay_t<F>;
    using copyable = copyable_func<Signature>;
    static_assert(std::is_constructible_v<VT, F>);
    if constexpr (std::is_function_v<std::remove_pointer_t<F>> ||
                  std::is_member_pointer_v<F> ||
                  func_internal::is_move_only_function_specialization_v<
                    std::remove_cvref_t<F>>) {
      if (func) {
        vptr = make<true, VT>(alloc, storage, std::forward<F>(func));
      } else {
        vptr = vtable::make_empty();
      }
    } else if constexpr (std::is_same_v<std::remove_cvref_t<F>, copyable>) {
      vptr = func.vptr;
      if constexpr (std::is_same_v<F, copyable>) {
        func.vptr->destructive_move(&func.storage, &storage);
        func.vptr = vtable::make_empty();
      } else {
        func.vptr->copy(allocator, &func.storage, &storage);
      }
    } else {
      vptr = make<true, VT>(alloc, storage, std::forward<F>(func));
    }
  }

  template<class T, class... Args>
    requires(std::is_constructible_v<std::decay_t<T>, Args && ...> &&
             is_callable_from<std::decay_t<T>>)
  constexpr explicit move_only_func(
    util::allocator_type auto &alloc,
    std::in_place_type_t<T>,
    Args &&...args
  )
    : vptr{ make<false, T>(alloc, storage, std::forward<Args>(args)...) }
    , allocator{ &alloc } {
    static_assert(std::is_same_v<T, std::decay_t<T>>);
  }

  template<class T, class U, class... Args>
    requires(std::is_constructible_v<
               std::decay_t<T>,
               std::initializer_list<U> &,
               Args && ...> &&
             is_callable_from<std::decay_t<T>>)
  constexpr explicit move_only_func(
    util::allocator_type auto &alloc,
    std::in_place_type_t<T>,
    std::initializer_list<U> ilist,
    Args &&...args
  )
    : vptr{ make<false, T>(
        alloc,
        storage,
        ilist,
        std::forward<Args>(args)...
      ) }
    , allocator{ &alloc } {
    static_assert(std::is_same_v<T, std::decay_t<T>>);
  }

  move_only_func(move_only_func const &) = delete;

  move_only_func(move_only_func &&other) noexcept {
    vtable::move_ctor(vptr, storage, other.vptr, other.storage);
  }

  auto operator=(move_only_func const &) -> move_only_func & = delete;

  auto operator=(move_only_func &&other) noexcept -> move_only_func & {
    if (&storage == &other.storage) { return *this; }

    vtable::move_assign(allocator, vptr, storage, other.vptr, other.storage);
    allocator = other.allocator;
    return *this;
  }

  auto operator=(std::nullptr_t) noexcept -> move_only_func & {
    if (*this) { move_only_func{}.swap(*this); }
    return *this;
  }

  // template<class F>
  // auto operator=(F &&func) -> move_only_func & {
  //   move_only_func{ allocator, std::forward<F>(func) }.swap(*this);
  //   return *this;
  // }

  ~move_only_func() noexcept { vptr->destroy(allocator, &storage); }

  using func_internal::function_call<move_only_func, Signature>::operator();

  explicit operator bool() const noexcept { return vptr->dispatch; }

  void swap(move_only_func &other) noexcept {
    vtable::swap(vptr, storage, other.vptr, other.storage);
  }

  friend void swap(move_only_func &lhs, move_only_func &rhs) noexcept {
    lhs.swap(rhs);
  }

  friend auto operator==(move_only_func const &self, std::nullptr_t) noexcept
    -> bool {
    return !self;
  }
};

template<class Signature>
class copyable_func<Signature> final
  : func_internal::function_call<copyable_func<Signature>, Signature> {

  using traits = func_internal::traits<FUNC_STORAGE_BYTES_COUNT, Signature>;
  using vtable = func_internal::vtable<FUNC_STORAGE_BYTES_COUNT, traits>;

  friend func_internal::function_call<copyable_func, Signature>;
  friend move_only_func<Signature>;

  template<class... T>
  static constexpr bool is_invocable_using{
    traits::template is_invocable_using<T...>
  };

  template<class VT>
  static constexpr bool is_callable_from{
    is_invocable_using<typename traits::template quals<VT>> &&
    is_invocable_using<typename traits::template inv_quals<VT>>
  };

  vtable const            *vptr{ vtable::make_empty() };
  typename vtable::context storage{};
  void                    *allocator{ nullptr };

  template<bool Copyable, class T, class... Args>
  static constexpr inline auto make(Args &&...args) -> decltype(auto) {
    return vtable::template make<Copyable, T>(std::forward<Args>(args)...);
  }

public:
  constexpr copyable_func(std::nullptr_t n = nullptr) noexcept {}

  template<class F>
    requires(
      !std::is_same_v<copyable_func, std::remove_cvref_t<F>> &&
      !func_internal::is_in_place_type_t_specialization_v<
        std::remove_cvref_t<F>> &&
      is_callable_from<std::decay_t<F>>
    )
  constexpr explicit copyable_func(
    util::allocator_type auto &alloc,
    F                        &&func
  ) noexcept
    : allocator{ &alloc } {
    using VT = std::decay_t<F>;

    static_assert(std::is_constructible_v<VT, F>);
    if constexpr (std::is_function_v<std::remove_pointer_t<F>> ||
                  std::is_member_pointer_v<F> ||
                  func_internal::is_copyable_function_specialization_v<
                    std::remove_cvref_t<F>>) {
      if (!func) { return; }
    }
    vptr = make<true, VT>(alloc, storage, std::forward<F>(func));
  }

  template<class T, class... Args>
    requires(std::is_constructible_v<std::decay_t<T>, Args && ...> &&
             is_callable_from<std::decay_t<T>>)
  constexpr explicit copyable_func(
    util::allocator_type auto &alloc,
    std::in_place_type_t<T>,
    Args &&...args
  )
    : vptr{ make<true, T>(alloc, storage, std::forward<Args>(args)...) }
    , allocator{ &alloc } {
    static_assert(std::is_same_v<T, std::decay_t<T>>);
  }

  template<class T, class U, class... Args>
    requires(std::is_constructible_v<
               std::decay_t<T>,
               std::initializer_list<U> &,
               Args && ...> &&
             is_callable_from<std::decay_t<T>>)
  constexpr explicit copyable_func(
    util::allocator_type auto &alloc,
    std::in_place_type_t<T>,
    std::initializer_list<U> ilist,
    Args &&...args
  )
    : vptr{ make<true, T>(alloc, storage, ilist, std::forward<Args>(args)...) }
    , allocator{ &alloc } {
    static_assert(std::is_same_v<T, std::decay_t<T>>);
  }

  constexpr copyable_func(copyable_func const &other)
    : vptr{ other.vptr }
    , allocator{ other.allocator } {
    other.vptr->copy(&other.storage, &storage);
  }

  constexpr copyable_func(copyable_func &&other) noexcept
    : allocator{ other.allocator } {
    vtable::move_ctor(vptr, storage, other.vptr, other.storage);
    other.allocator = nullptr;
  }

  auto operator=(copyable_func const &other) -> copyable_func & {
    if (this == &other) { return *this; }

    vptr->destroy(allocator, &storage);
    if (!other.vptr->is_noexcept_copyable) {
      typename vtable::context tmp;
      other.vptr->copy(&other.storage, &tmp);
      other.vptr->move(&tmp, &storage);
    } else {
      other.vptr->copy(&other.storage, &storage);
    }
    vptr      = other.vptr;
    allocator = other.allocator;
    return *this;
  }

  auto operator=(copyable_func &&other) noexcept -> copyable_func & {
    if (&storage == &other.storage) { return *this; }

    vtable::move_assign(vptr, storage, other.vptr, other.storage);
    allocator = other.allocator;
    return *this;
  }

  auto operator=(std::nullptr_t) noexcept -> copyable_func & {
    if (*this) { copyable_func{}.swap(*this); }
    return *this;
  }

  // template<class F>
  // auto operator=(F &&func) -> copyable_func & {
  //   copyable_func{ allocator, std::forward<F>(func) }.swap(*this);
  //   return *this;
  // }

  ~copyable_func() noexcept { vptr->destroy(allocator, &storage); }

  using func_internal::function_call<copyable_func, Signature>::operator();

  constexpr explicit operator bool() const noexcept { return vptr->dispatch; }

  constexpr void swap(copyable_func &other) noexcept {
    vtable::swap(vptr, storage, other.vptr, other.storage);
  }

  friend void swap(copyable_func &lhs, copyable_func &rhs) noexcept {
    lhs.swap(rhs);
  }

  friend auto operator==(copyable_func const &self, std::nullptr_t) noexcept
    -> bool {
    return !self;
  }
};

} // namespace gzn::fnd
