#pragma once
#include <type_traits>

struct bad_function_call : std::exception {
  char const *what() const noexcept override {
    return "bad function call";
  }
};

namespace functional_ {
static constexpr size_t MAX_SIZE_OF = sizeof(void *);
static constexpr size_t MAX_ALIGN_OF = alignof(void *);

template<typename T>
static constexpr bool is_small_obj =
    sizeof(T) < MAX_SIZE_OF
        && std::is_nothrow_move_constructible_v<T>; // alignof(T) < MAX_ALIGN_OF is true if sizeof(T) < MAX_SIZE_OF

using st_type = std::aligned_storage_t<sizeof(void *), alignof(void *)>;

template<typename R, typename... Args>
struct storage;

template<typename R, typename... Args>
struct operations_interface {
  using storage = functional_::storage<R, Args...>;

  void (*copy)(storage *, storage const *);
  void (*move)(storage *, storage *) noexcept;
  R (*apply)(storage const *, Args...);
  void (*destroy)(storage *);
};

template<typename R, typename... Args>
operations_interface<R, Args...> const *get_empty_type_operations() {
  using storage = functional_::storage<R, Args...>;
  static constexpr operations_interface<R, Args...> operations = {
      /* copy */
      [](storage *dst, storage const *) {
        dst->ops = get_empty_type_operations<R, Args...>();
      },
      /* move */
      [](storage *dst, storage *) noexcept {
        dst->ops = get_empty_type_operations<R, Args...>();
      },
      /* apply */
      [](storage const *, Args...) -> R {
        throw bad_function_call();
      },
      /* destroy */
      [](storage *) {}
  };
  return &operations;
}

template<typename F, typename = void>
struct object_traits;

template<typename F>
struct object_traits<F, std::enable_if_t<is_small_obj<F>>> {

  template<typename R, typename... Args>
  static operations_interface<R, Args...> const *get_operations() {

    using storage = functional_::storage<R, Args...>;

    static constexpr operations_interface<R, Args...> operations = {
        /* copy */
        [](storage *dst, storage const *src) {
          new(&dst->small) F(src->template get_obj<F>());
          dst->ops = src->ops;
        },
        /* move */
        [](storage *dst, storage *src) noexcept {
          new(&dst->small) F(std::move(src->template get_obj<F>()));
          dst->ops = src->ops;
        },
        /* apply */
        [](storage const *dst, Args... args) -> R {
          return (dst->template get_obj<F>())(std::forward<Args>(args)...);
        },
        /* destroy */
        [](storage *dst) {
          dst->template get_obj<F>().~F();
        }
    };
    return &operations;
  }

  template<typename R, typename... Args>
  static void init(storage<R, Args...> &storage, F &&func) {
    new(&storage.small) F(std::move(func));
  }

  template<typename R, typename... Args>
  static F *target(storage<R, Args...> &st) noexcept {
    return &(st.template get_obj<F>());
  }

  template<typename R, typename... Args>
  static F const *target(storage<R, Args...> const &st) noexcept {
    return &(st.template get_obj<F>());
  }
};

template<typename F>
struct object_traits<F, std::enable_if_t<!is_small_obj<F>>> {

  template<typename R, typename... Args>
  static operations_interface<R, Args...> const *get_operations() {

    using storage = functional_::storage<R, Args...>;

    static constexpr operations_interface<R, Args...> operations = {
        /* copy */
        [](storage *dst, storage const *src) {
          dst->ops = src->ops;
          dst->set(new F(*src->template get<F>()));
        },
        /* move */
        [](storage *dst, storage *src) noexcept {
          dst->ops = src->ops;
          dst->set((void *) src->template get<F>());
          src->ops = get_empty_type_operations<R, Args...>();
        },
        /* apply */
        [](storage const *dst, Args... args) -> R {
          return (*dst->template get<F>())(std::forward<Args...>(args)...);
        },
        /* destroy */
        [](storage *dst) {
          delete dst->template get<F>();
        }
    };
    return &operations;
  }

  template<typename R, typename... Args>
  static void init(storage<R, Args...> &storage, F &&func) {
    storage.set(new F(std::move(func)));
  }

  template<typename R, typename... Args>
  static F *target(storage<R, Args...> &st) noexcept {
    return st.template get<F>();
  }

  template<typename R, typename... Args>
  static F const *target(storage<R, Args...> const &st) noexcept {
    return st.template get<F>();
  }
};

template<typename R, typename... Args>
struct storage {
  template<typename F>
  F *get() const noexcept {
    return static_cast<F *>(reinterpret_cast<void *const &>(small));
  }

  template<typename F>
  F &get_obj() noexcept {
    return reinterpret_cast<F &>(small);
  }

  template<typename F>
  F const &get_obj() const noexcept {
    return reinterpret_cast<F const &>(small);
  }

  void set(void *t) {
    reinterpret_cast<void *&>(small) = t;
  }

  operations_interface<R, Args...> const *ops;
  st_type small;
};
} // namespace functional_

template<typename F>
struct function;

template<typename R, typename... Args>
struct function<R(Args...)> {
  function() noexcept {
    storage.ops = functional_::get_empty_type_operations<R, Args...>();
  };

  function(function const &other) {
    other.storage.ops->copy(&storage, &other.storage);
  }

  function(function &&other) noexcept {
    other.storage.ops->move(&storage, &other.storage);
  }

  template<typename F>
  function(F f) {
    using traits = functional_::object_traits<F>;
    traits::template init<R, Args...>(storage, std::move(f));
    storage.ops = traits::template get_operations<R, Args...>();
  }

  function &operator=(function const &rhs) {
    if (&rhs == this)
      return *this;
    function(rhs).swap(*this);
    return *this;
  }

  function &operator=(function &&rhs) noexcept {
    if (&rhs == this)
      return *this;
    rhs.swap(*this);
    return *this;
  }

  ~function() {
    storage.ops->destroy(&storage);
  }

  explicit operator bool() const noexcept {
    return storage.ops != functional_::get_empty_type_operations<R, Args...>();
  }

  R operator()(Args... args) const {
    return storage.ops->apply(&storage, std::forward<Args>(args)...);
  }

  template<typename T>
  T *target() noexcept {
    using traits = functional_::object_traits<T>;

    if (storage.ops == traits::template get_operations<R, Args...>())
      return traits::template target(storage);
    else
      return nullptr;
  }

  template<typename T>
  T const *target() const noexcept {
    using traits = functional_::object_traits<T>;

    if (storage.ops == traits::template get_operations<R, Args...>())
      return traits::template target(storage);
    else
      return nullptr;
  }

  void swap(function &other) {
    std::swap(storage, other.storage);
  }

 private:
  functional_::storage<R, Args...> storage;
};
