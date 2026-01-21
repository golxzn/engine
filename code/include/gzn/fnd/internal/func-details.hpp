/** This file is slightly customized fusion of the following tremendous works:
 *  1. https://github.com/electronicarts/EASTL
 *  2. https://github.com/MFHava/P2548.git
 *  3. https://github.com/skarupke/std_function
 *  And I bet this implementation slower than all of them...
 */
#pragma once

#include <functional>
#include <utility>

#include "gzn/fnd/allocators.hpp"

namespace gzn::fnd::func_internal {

#define FUNC_MOVE(value) \
  static_cast<typename std::remove_reference<decltype(value)>::type &&>(value)
#define FUNC_FORWARD(type, value) static_cast<type &&>(value)

class unused_class {};

union functor_storage_alignment {
  void (*unused_func_ptr)(void);
  void (unused_class::*unused_func_mem_ptr)(void);
  void *unused_ptr;
  s64   unused_var;
};

usize inline constexpr MINIMUM_STORAGE_BYTES_COUNT{
  sizeof(functor_storage_alignment)
};

template<usize BytesCount>
struct functor_storage final {
  usize static constexpr bytes_count{ BytesCount };
  gzn_static_assert(
    BytesCount >= MINIMUM_STORAGE_BYTES_COUNT,
    "Local buffer storage should be at least this size"
  );

  union {
    functor_storage_alignment alignment;
    void                     *ptr;
    c_array<byte, BytesCount> sbo;
  };
};

/// @note maybe just std::in_place_type_t
template<class Context, class Functor>
bool inline constexpr is_functor_inplace_allocatable_v{
  sizeof(Functor) <= Context::bytes_count &&
  (alignof(Context) % alignof(Functor)) == 0
};

template<class Functor, usize BytesCount>
bool inline constexpr is_sbo{ sizeof(Functor) <= BytesCount &&
                              std::is_nothrow_move_constructible_v<Functor> };

template<usize BytesCount, class Traits>
struct vtable final {
  using dispatch_fn          = typename Traits::dispatch_type;
  using context              = functor_storage<BytesCount>;
  using copy_fn              = bool (*)(void *, context *, context *) noexcept;
  using destroy_fn           = void (*)(void *, context *) noexcept;
  using destructive_move_fn  = void (*)(context *, context *) noexcept;
  using noexcept_copyable_fn = bool (*)() noexcept;

  dispatch_fn         dispatch{ nullptr };
  copy_fn             copy{ nullptr };
  destroy_fn          destroy{ nullptr };
  destructive_move_fn destructive_move{ nullptr };
  bool                is_noexcept_copyable{ false };

  static void move_ctor(
    vtable const *&lhs_vptr,
    context       &lhs_storage,
    vtable const *&rhs_vptr,
    context       &rhs_storage
  ) noexcept {
    lhs_vptr = rhs_vptr;
    rhs_vptr->destructive_move(&rhs_storage, &lhs_storage);
    rhs_vptr = vtable::make_empty();
  }

  static void move_assign(
    void          *allocator,
    vtable const *&lhs_vptr,
    context       &lhs_storage,
    vtable const *&rhs_vptr,
    context       &rhs_storage
  ) noexcept {
    lhs_vptr->destroy(allocator, &lhs_storage);
    lhs_vptr = rhs_vptr;
    rhs_vptr->destructive_move(&rhs_storage, &lhs_storage);
    rhs_vptr = vtable::make_empty();
  }

  static void swap(
    vtable const *&lhs_vptr,
    context       &lhs_storage,
    vtable const *&rhs_vptr,
    context       &rhs_storage
  ) noexcept {
    if (&lhs_storage == &rhs_storage) { return; }

    context tmp;
    lhs_vptr->destructive_move(&lhs_storage, &tmp);
    rhs_vptr->destructive_move(&rhs_storage, &lhs_storage);
    lhs_vptr->destructive_move(&tmp, &rhs_storage);
    std::swap(lhs_vptr, rhs_vptr);
  }

  template<
    bool Copyable,
    bool InPlace,
    class T,
    util::allocator_type Allocator>
  struct functor_manager final {
    template<class... Args>
      requires std::is_constructible_v<T, Args &&...>
    static constexpr void construct(
      void    *alloc,
      context &storage,
      Args &&...args
    ) noexcept(std::is_nothrow_constructible_v<T, Args &&...>) {
      if constexpr (Copyable) {
        gzn_static_assert(std::is_copy_constructible_v<T>);
      }
      if constexpr (!InPlace) {
        storage.ptr = static_cast<Allocator *>(alloc)->allocate(
          sizeof(T), alignof(T), 0u, 0u
        );
        new (storage.ptr) T{ FUNC_FORWARD(Args, args)... };
      } else {
        new (storage.sbo) T{ FUNC_FORWARD(Args, args)... };
      }
    }

    static constexpr auto copy(
      void    *alloc,
      context *from,
      context *to
    ) noexcept(std::is_nothrow_copy_constructible_v<T>) -> bool {
      if constexpr (!Copyable) { std::unreachable(); }

      if constexpr (!InPlace) {
        to->ptr = static_cast<Allocator *>(alloc)->allocate(
          sizeof(T), alignof(T), 0u, 0u
        );
        new (to->ptr) T{ *reinterpret_cast<T const *>(from->ptr) };
      } else {
        new (to->sbo) T{ *reinterpret_cast<T const *>(from->ptr) };
      }
      return true;
    }

    static constexpr void destroy(void *alloc, context *target) noexcept(
      std::is_nothrow_destructible_v<T>
    ) {
      reinterpret_cast<T *>(target->ptr)->~T();
      if constexpr (!InPlace) {
        if (alloc) {
          static_cast<Allocator *>(alloc)->deallocate(target->ptr, sizeof(T));
        }
      }
    }

    static constexpr void
    destructive_move(context *from, context *to) noexcept(
      std::is_nothrow_move_constructible_v<T> &&
      std::is_nothrow_destructible_v<T>
    ) {
      if constexpr (InPlace) {
        new (to->sbo) T{ FUNC_MOVE(*reinterpret_cast<T *>(from->sbo)) };
        reinterpret_cast<T *>(from->sbo)->~T();
      } else {
        to->ptr = std::exchange(from->ptr, nullptr);
      }
    }
  };

  struct nop_functor_manager final {
    static constexpr auto copy(void *, context *, context *) noexcept -> bool {
      return false;
    }

    static constexpr void destroy(void *, context *) noexcept {}

    static constexpr void destructive_move(context *, context *) noexcept {}
  };

  template<bool Copyable, class T, class... Args>
  static auto make(
    util::allocator_type auto &alloc,
    context                   &storage,
    Args &&...args
  ) -> vtable const * {
    using allocator_type = std::remove_cvref_t<decltype(alloc)>;
    auto constexpr in_place{ is_functor_inplace_allocatable_v<context, T> };
    using manager_type =
      functor_manager<Copyable, in_place, T, allocator_type>;

    manager_type::template construct<Args...>(
      &alloc, storage, FUNC_FORWARD(Args, args)...
    );

    static constexpr vtable _static_vtable{
      .dispatch         = &Traits::template dispatch<T, is_sbo<T, BytesCount>>,
      .copy             = manager_type::copy,
      .destroy          = manager_type::destroy,
      .destructive_move = manager_type::destructive_move,
      .is_noexcept_copyable = std::is_nothrow_copy_constructible_v<T>,
    };
    return &_static_vtable;
  }

  static auto make_empty() -> vtable const * {
    vtable static constexpr _static_vtable{
      .dispatch             = &Traits::dispatch_empty,
      .copy                 = nop_functor_manager::copy,
      .destroy              = nop_functor_manager::destroy,
      .destructive_move     = nop_functor_manager::destructive_move,
      .is_noexcept_copyable = false
    };
    return &_static_vtable;
  }
};

template<
  usize BytesCount,
  bool  Const,
  bool  Noexcept,
  bool  Move,
  class Result,
  class... Args>
class invoker {
  using context = functor_storage<BytesCount>;

  template<class T>
  using const_ = std::conditional_t<Const, T const, T>;

  template<class T>
  using move_ = std::conditional_t<Move, T &&, T &>;

  template<class T>
  inline static constexpr auto move(T &&t) noexcept -> decltype(auto) {
    if constexpr (Move) {
      return FUNC_MOVE(t);
    } else {
      return (t);
    }
  }

  template<class T, bool SBO>
  inline static constexpr auto get(const_<context> *ctx) noexcept
    -> move_<const_<T>> {
    if constexpr (SBO) {
      return move(*reinterpret_cast<const_<T> *>(ctx->sbo));
    } else {
      return move(*reinterpret_cast<const_<T> *>(ctx->ptr));
    }
  }

public:
  using dispatch_type = std::conditional_t<
    Noexcept,
    Result (*)(Args..., const_<context> *) noexcept,
    Result (*)(Args..., const_<context> *)>;

  template<class T, bool SBO>
  static constexpr auto dispatch(Args... args, const_<context> *ctx) noexcept(
    Noexcept
  ) -> Result {
    return std::invoke_r<Result>(
      get<T, SBO>(ctx), FUNC_FORWARD(Args, args)...
    );
  }

  static constexpr auto dispatch_empty(Args..., const_<context> *) noexcept(
    Noexcept
  ) -> Result {
    gzn_do_assertion("Null functor call");
    if constexpr (!std::is_same_v<void, Result>) { return Result{}; }
  }
};

template<class Signature>
struct invocable_using;

template<class Result, class... Args>
struct invocable_using<Result(Args...)> {
  template<class... T>
  bool static constexpr is_invocable_using{
    std::is_invocable_r_v<Result, T..., Args...>
  };
};

template<class Result, class... Args>
struct invocable_using<Result(Args...) noexcept> {
  template<class... T>
  bool static constexpr is_invocable_using{
    std::is_nothrow_invocable_r_v<Result, T..., Args...>
  };
};

bool inline constexpr is_const{ true };
bool inline constexpr is_not_const{ !is_const };
bool inline constexpr is_noexcept{ true };
bool inline constexpr is_not_noexcept{ !is_noexcept };
bool inline constexpr is_movable{ true };
bool inline constexpr is_not_movable{ !is_movable };

template<usize, class>
struct traits;

template<usize BytesCount, class Result, class... Args>
struct traits<BytesCount, Result(Args...)> final
  : invocable_using<Result(Args...)>
  , invoker<
      BytesCount,
      is_not_const,
      is_not_noexcept,
      is_not_movable,
      Result,
      Args...> {
  template<class VT>
  using quals = VT;

  template<class VT>
  using inv_quals = VT &;
};

template<usize BytesCount, class Result, class... Args>
struct traits<BytesCount, Result(Args...) const> final
  : invocable_using<Result(Args...)>
  , invoker<
      BytesCount,
      is_const,
      is_not_noexcept,
      is_not_movable,
      Result,
      Args...> {
  template<class VT>
  using quals = const VT;

  template<class VT>
  using inv_quals = const VT &;
};

template<usize BytesCount, class Result, class... Args>
struct traits<BytesCount, Result(Args...) noexcept> final
  : invocable_using<Result(Args...) noexcept>
  , invoker<
      BytesCount,
      is_not_const,
      is_noexcept,
      is_not_movable,
      Result,
      Args...> {
  template<class VT>
  using quals = VT;

  template<class VT>
  using inv_quals = VT &;
};

template<usize BytesCount, class Result, class... Args>
struct traits<BytesCount, Result(Args...) const noexcept> final
  : invocable_using<Result(Args...) noexcept>
  , invoker<
      BytesCount,
      is_const,
      is_noexcept,
      is_not_movable,
      Result,
      Args...> {
  template<class VT>
  using quals = const VT;

  template<class VT>
  using inv_quals = const VT &;
};

template<usize BytesCount, class Result, class... Args>
struct traits<BytesCount, Result(Args...) &> final
  : invocable_using<Result(Args...)>
  , invoker<
      BytesCount,
      is_not_const,
      is_not_noexcept,
      is_not_movable,
      Result,
      Args...> {
  template<class VT>
  using quals = VT &;

  template<class VT>
  using inv_quals = quals<VT>;
};

template<usize BytesCount, class Result, class... Args>
struct traits<BytesCount, Result(Args...) const &> final
  : invocable_using<Result(Args...)>
  , invoker<
      BytesCount,
      is_const,
      is_not_noexcept,
      is_not_movable,
      Result,
      Args...> {
  template<class VT>
  using quals = const VT &;

  template<class VT>
  using inv_quals = quals<VT>;
};

template<usize BytesCount, class Result, class... Args>
struct traits<BytesCount, Result(Args...) & noexcept> final
  : invocable_using<Result(Args...) noexcept>
  , invoker<
      BytesCount,
      is_not_const,
      is_noexcept,
      is_not_movable,
      Result,
      Args...> {
  template<class VT>
  using quals = VT &;

  template<class VT>
  using inv_quals = quals<VT>;
};

template<usize BytesCount, class Result, class... Args>
struct traits<BytesCount, Result(Args...) const & noexcept> final
  : invocable_using<Result(Args...) noexcept>
  , invoker<
      BytesCount,
      is_const,
      is_noexcept,
      is_not_movable,
      Result,
      Args...> {
  template<class VT>
  using quals = const VT &;

  template<class VT>
  using inv_quals = quals<VT>;
};

template<usize BytesCount, class Result, class... Args>
struct traits<BytesCount, Result(Args...) &&> final
  : invocable_using<Result(Args...)>
  , invoker<
      BytesCount,
      is_not_const,
      is_not_noexcept,
      is_movable,
      Result,
      Args...> {
  template<class VT>
  using quals = VT &&;

  template<class VT>
  using inv_quals = quals<VT>;
};

template<usize BytesCount, class Result, class... Args>
struct traits<BytesCount, Result(Args...) const &&> final
  : invocable_using<Result(Args...)>
  , invoker<
      BytesCount,
      is_const,
      is_not_noexcept,
      is_movable,
      Result,
      Args...> {
  template<class VT>
  using quals = const VT &&;

  template<class VT>
  using inv_quals = quals<VT>;
};

template<usize BytesCount, class Result, class... Args>
struct traits<BytesCount, Result(Args...) && noexcept> final
  : invocable_using<Result(Args...) noexcept>
  , invoker<
      BytesCount,
      is_not_const,
      is_noexcept,
      is_movable,
      Result,
      Args...> {
  template<class VT>
  using quals = VT &&;

  template<class VT>
  using inv_quals = quals<VT>;
};

template<usize BytesCount, class Result, class... Args>
struct traits<BytesCount, Result(Args...) const && noexcept> final
  : invocable_using<Result(Args...) noexcept>
  , invoker<BytesCount, is_const, is_noexcept, is_movable, Result, Args...> {
  template<class VT>
  using quals = const VT &&;

  template<class VT>
  using inv_quals = quals<VT>;
};


template<class Impl, class Signature>
struct function_call;

template<class Impl, class Result, class... Args>
struct function_call<Impl, Result(Args...)> {
  constexpr auto operator()(Args... args) -> Result {
    auto &self{ *static_cast<Impl *>(this) };
    return self.vptr->dispatch(FUNC_FORWARD(Args, args)..., &self.storage);
  }
};

template<class Impl, class Result, class... Args>
struct function_call<Impl, Result(Args...) const> {
  constexpr auto operator()(Args... args) const -> Result {
    auto &self{ *static_cast<Impl const *>(this) };
    return self.vptr->dispatch(FUNC_FORWARD(Args, args)..., &self.storage);
  }
};

template<class Impl, class Result, class... Args>
struct function_call<Impl, Result(Args...) noexcept> {
  constexpr auto operator()(Args... args) noexcept -> Result {
    auto &self{ *static_cast<Impl *>(this) };
    return self.vptr->dispatch(FUNC_FORWARD(Args, args)..., &self.storage);
  }
};

template<class Impl, class Result, class... Args>
struct function_call<Impl, Result(Args...) const noexcept> {
  constexpr auto operator()(Args... args) const noexcept -> Result {
    auto &self{ *static_cast<Impl const *>(this) };
    return self.vptr->dispatch(FUNC_FORWARD(Args, args)..., &self.storage);
  }
};

template<class Impl, class Result, class... Args>
struct function_call<Impl, Result(Args...) &> {
  constexpr auto operator()(Args... args) & -> Result {
    auto &self{ *static_cast<Impl *>(this) };
    return self.vptr->dispatch(FUNC_FORWARD(Args, args)..., &self.storage);
  }
};

template<class Impl, class Result, class... Args>
struct function_call<Impl, Result(Args...) const &> {
  constexpr auto operator()(Args... args) const & -> Result {
    auto &self{ *static_cast<Impl const *>(this) };
    return self.vptr->dispatch(FUNC_FORWARD(Args, args)..., &self.storage);
  }
};

template<class Impl, class Result, class... Args>
struct function_call<Impl, Result(Args...) & noexcept> {
  constexpr auto operator()(Args... args) & noexcept -> Result {
    auto &self{ *static_cast<Impl *>(this) };
    return self.vptr->dispatch(FUNC_FORWARD(Args, args)..., &self.storage);
  }
};

template<class Impl, class Result, class... Args>
struct function_call<Impl, Result(Args...) const & noexcept> {
  constexpr auto operator()(Args... args) const & noexcept -> Result {
    auto &self{ *static_cast<Impl const *>(this) };
    return self.vptr->dispatch(FUNC_FORWARD(Args, args)..., &self.storage);
  }
};

template<class Impl, class Result, class... Args>
struct function_call<Impl, Result(Args...) &&> {
  constexpr auto operator()(Args... args) && -> Result {
    auto &self{ *static_cast<Impl *>(this) };
    return self.vptr->dispatch(FUNC_FORWARD(Args, args)..., &self.storage);
  }
};

template<class Impl, class Result, class... Args>
struct function_call<Impl, Result(Args...) const &&> {
  constexpr auto operator()(Args... args) const && -> Result {
    auto &self{ *static_cast<Impl const *>(this) };
    return self.vptr->dispatch(FUNC_FORWARD(Args, args)..., &self.storage);
  }
};

template<class Impl, class Result, class... Args>
struct function_call<Impl, Result(Args...) && noexcept> {
  constexpr auto operator()(Args... args) && noexcept -> Result {
    auto &self{ *static_cast<Impl *>(this) };
    return self.vptr->dispatch(FUNC_FORWARD(Args, args)..., &self.storage);
  }
};

template<class Impl, class Result, class... Args>
struct function_call<Impl, Result(Args...) const && noexcept> {
  constexpr auto operator()(Args... args) const && noexcept -> Result {
    auto &self{ *static_cast<Impl const *>(this) };
    return self.vptr->dispatch(FUNC_FORWARD(Args, args)..., &self.storage);
  }
};

#undef FUNC_MOVE
#undef FUNC_FORWARD

} // namespace gzn::fnd::func_internal
