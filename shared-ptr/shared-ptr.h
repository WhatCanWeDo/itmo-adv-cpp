#pragma once
#include <cstddef>
#include <iostream>
#include <memory>

template <typename T>
class shared_ptr;

template <typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args);
namespace shared_ptr_details {

class control_block {
  size_t strong_cnt{0};
  size_t weak_cnt{0};

  template <typename T>
  friend class shared_ptr;

  template <typename T>
  friend class weak_ptr;

public:
  control_block() noexcept;
  void inc_strong() noexcept;
  void inc_weak() noexcept;
  void dec_strong() noexcept;
  void dec_weak() noexcept;
  size_t get_strong_cnt() const noexcept;
  size_t get_weak_cnt() const noexcept;
  virtual void delete_data() = 0;
  virtual ~control_block();
};

template <typename T, class D>
class ptr_block : public control_block, public D {
  T* ptr;

public:
  ptr_block(T* ptr_, D d) : control_block(), D(std::move(d)), ptr(ptr_) {
    inc_strong();
  }

  void delete_data() override {
    static_cast<D&> (*this)(ptr);
  }
};

template <typename T>
class obj_block : public control_block {
  std::aligned_storage_t<sizeof(T), alignof(T)> obj;

  template <typename U, typename... Args>
  friend ::shared_ptr<U> (::make_shared(Args&&... args));

public:
  template <typename... Args>
  obj_block(Args&&... args) : control_block() {
    new (&obj) T(std::forward<Args>(args)...);
  }

  void delete_data() override {
    reinterpret_cast<T*>(&obj)->~T();
  }
};
}
template <typename T>
class shared_ptr {
  T* ptr{nullptr};
  shared_ptr_details::control_block* block{nullptr};

  shared_ptr(shared_ptr_details::control_block* block_, T* ptr_) {
    try {
      ptr = ptr_;
      block = block_;
    } catch(...) {
      delete ptr;
      throw;
    }
    if (block) {
      block->inc_strong();
    }
  }

  template<typename U, typename... Args>
  friend shared_ptr<U> make_shared(Args&& ... args);

  template<typename U>
  friend class shared_ptr;

  template<typename U>
  friend class weak_ptr;

public:
  shared_ptr() noexcept = default;
  explicit shared_ptr(std::nullptr_t ptr_) noexcept : ptr(ptr_), block(nullptr) {}

  template<typename U, class D = std::default_delete<U>,
            typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
  shared_ptr(U* ptr_, D d = D()) : ptr(ptr_), block(new shared_ptr_details::ptr_block<U, D>(ptr_, std::move(d))) {}

  template <typename U,
            typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
  shared_ptr(const shared_ptr<U>& other) noexcept: shared_ptr(other.block, other.ptr) {}

  template <typename U>
  shared_ptr(const shared_ptr<U> &other, T* ptr_) noexcept : shared_ptr(other.block, ptr_) {}

  shared_ptr(const shared_ptr &other) noexcept : shared_ptr(other.block, other.ptr) {}

  shared_ptr(shared_ptr&& other) noexcept : shared_ptr() {
    swap(other);
  }

  shared_ptr& operator=(const shared_ptr& other) noexcept {
      if (this == &other) return *this;
      shared_ptr tmp = other;
      swap(tmp);
      return *this;
  }

  shared_ptr& operator=(shared_ptr&& other) noexcept {
      if (this == &other) return *this;
      reset();
      swap(other);
      return *this;
  }

  T* get() const noexcept {
      return ptr;
  }

  operator bool() const noexcept {
      return ptr != nullptr;
  }

  T& operator*() const noexcept {
    return *ptr;
  }

  T* operator->() const noexcept {
    return ptr;
  }

  std::size_t use_count() const noexcept {
    if (block) return block->get_strong_cnt();
    return 0;
  }

  void reset() noexcept {
      shared_ptr().swap(*this);
  }


  template <typename U, class D = std::default_delete<U>,
      typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
  void reset(U* new_ptr, D d = D()) {
      shared_ptr<T>(new_ptr, d).swap(*this);
  }

  void swap(shared_ptr &other) {
    std::swap(ptr, other.ptr);
    std::swap(block, other.block);
  }

  ~shared_ptr() {
    if (block) {
      block->dec_strong();
    }
    block = nullptr;
    ptr = nullptr;
  }
};

template <typename T, typename U>
bool operator==(const shared_ptr<T> &a, const shared_ptr<U> &b) {
  return a.get() == b.get();
}

template <typename T, typename U>
bool operator!=(const shared_ptr<T> &a, const shared_ptr<U> &b) {
  return a.get() != b.get();
}

template <typename T>
bool operator==(const shared_ptr<T> &a, std::nullptr_t b) {
  return a.get() == b;
}

template <typename T>
bool operator!=(const shared_ptr<T> &a, std::nullptr_t b) {
  return a.get() != b;
}

template <typename T>
bool operator==(std::nullptr_t a, const shared_ptr<T> &b) {
  return a == b.get();
}

template <typename T>
bool operator!=(std::nullptr_t a, const shared_ptr<T> &b) {
  return a != b.get();
}

template <typename T>
class weak_ptr {
  T* ptr{nullptr};
  shared_ptr_details::control_block* block{nullptr};
  weak_ptr(T* ptr_, shared_ptr_details::control_block* block_) noexcept : ptr(ptr_), block(block_){
    if (block) {
      block->inc_weak();
    }
  }
public:
  weak_ptr() noexcept = default;

  template<typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
  weak_ptr(const shared_ptr<U>& other) noexcept : weak_ptr(other.ptr, other.block) {}

  template<typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
  weak_ptr(const weak_ptr<U>& other) noexcept : weak_ptr(other.ptr, other.block) {}

  weak_ptr(const shared_ptr<T> &other) noexcept : weak_ptr(other.ptr, other.block) {}
  weak_ptr(const weak_ptr &other) noexcept : weak_ptr(other.ptr, other.block) {}

  weak_ptr& operator=(const shared_ptr<T>& other) noexcept {
    weak_ptr(other).swap(*this);
    return *this;
  }

  weak_ptr& operator=(const weak_ptr& other) noexcept {
    if (&other == this) return *this;
    weak_ptr(other).swap(*this);
    return *this;
  }

  weak_ptr& operator=(weak_ptr&& other) noexcept {
    if (&other == this) return *this;
    other.swap(*this);
    weak_ptr().swap(other);
    return *this;
  }

  shared_ptr<T> lock() const noexcept {
      if (block && block->get_strong_cnt() == 0) {
        return shared_ptr<T>();
      }
      return shared_ptr<T>(block, ptr);
  }

  ~weak_ptr() {
    if (block) {
      block->dec_weak();
    }
    ptr = nullptr;
    block = nullptr;
  }

  void swap(weak_ptr& other) {
    std::swap(other.ptr, ptr);
    std::swap(other.block, block);
  }
};

template <typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
  auto* block = new shared_ptr_details::obj_block<T>(std::forward<Args>(args)...);
  return shared_ptr<T>(block, reinterpret_cast<T*>(&block->obj));
}
