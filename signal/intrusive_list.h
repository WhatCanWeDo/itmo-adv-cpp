#pragma once

#include <algorithm>
#include <cassert>
#include <type_traits>

namespace intrusive {
struct default_tag;

struct base_list_element {
  base_list_element();
  base_list_element(const base_list_element&) = delete;
  base_list_element(base_list_element&& other);

  base_list_element& operator=(base_list_element&& other);

  void unlink();

  void insert_before(base_list_element* pos);

  bool in_list() const noexcept;
  ~base_list_element();
private:
  base_list_element* prev{nullptr};
  base_list_element* next{nullptr};
  template <typename T, typename Tag>
  friend struct list;
};

template <typename Tag = default_tag>
struct list_element : base_list_element{};


template <typename T, typename Tag = default_tag>
struct list {
  static_assert(std::is_base_of_v<list_element<Tag>, T>, "You should derive from list_element");
  template <bool is_const>
  struct iterators_impl;
  using iterator = iterators_impl<false>;
  using const_iterator = iterators_impl<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  list() noexcept {
    sentinel.next = sentinel.prev = &sentinel;
  }

  list(list const&) = delete;
  list& operator=(list const&) = delete;

  list(list&& other) noexcept {
    move_sentinel(std::move(other));
  }

  list& operator=(list&& other) noexcept {
    move_sentinel(std::move(other));
    return *this;
  }

  T& front() noexcept {
    return static_cast<T&>(*sentinel.next);
  }

  T& back() noexcept {
    return static_cast<T&>(*sentinel.prev);
  }

  T const& front() const noexcept {
    return static_cast<T const&>(*sentinel.next);
  }

  T const& back() const noexcept {
    return static_cast<T const&>(*sentinel.prev);
  }

  void pop_front() noexcept {
    sentinel.next->unlink();
  }

  void pop_back() noexcept {
    sentinel.prev->unlink();
  }

  bool empty() const noexcept {
    return sentinel.next == sentinel.prev && sentinel.next == &sentinel;
  }
  void splice(const_iterator pos, list& l,
              const_iterator first,
              const_iterator last) noexcept {

    if (pos == last || first == last) {
      return;
    }
    base_list_element* tmp = last.ptr->prev;
    last.ptr->prev = first.ptr->prev;
    if (first.ptr->prev) {
      first.ptr->prev->next = last.ptr;
    }
    if (pos->prev) {
      pos.ptr->prev->next = first.ptr;
    }
    first.ptr->prev = pos.ptr->prev;
    pos.ptr->prev = tmp;
    tmp->next = pos.ptr;
  }

  iterator erase(iterator it) {
    iterator res = iterator(static_cast<list_element<Tag>*>(it.ptr->next));
    it->unlink();
    return res;
  }


  /**
   * inserts element before position pos
   * @param pos
   * @param elem
   */
  iterator insert(iterator pos, T& elem) {
    if (&to_base(elem) == pos.ptr) {
      return pos;
    }
    if (to_base(elem).in_list()) {
      to_base(elem).unlink();
    }
    to_base(elem).insert_before(pos.ptr);
    return iterator(static_cast<list_element<Tag>*>(pos.ptr->prev));
  }

  void push_back(T& elem) {
    insert(end(), elem);
  }

  void push_front(T& elem) {
    insert(begin(), elem);
  }

  iterator as_iterator(T& element) noexcept {
    return iterator(static_cast<list_element<Tag>*>(&element));
  }

  const_iterator as_iterator(T& element) const noexcept {
    return const_iterator(static_cast<list_element<Tag>*>(&element));
  }

  iterator begin() noexcept{
    return iterator(static_cast<list_element<Tag>*>(sentinel.next));
  }

  iterator end() noexcept{
    return iterator(&sentinel);
  }

  const_iterator begin() const noexcept{
    return const_iterator(static_cast<list_element<Tag>*>(sentinel.next));
  }

  const_iterator end() const noexcept {
    return const_iterator(&sentinel);
  }

private:
  static list_element<Tag>& to_base(T& elem) {
    return static_cast<list_element<Tag>&>(elem);
  }

  void move_sentinel(list&& other) {
    if (other.empty()) {
      sentinel.next = sentinel.prev = &sentinel;
      return;
    }
    sentinel = std::move(other.sentinel);
    if (sentinel.next) {
      sentinel.next->prev = &sentinel;
    }
    if (sentinel.prev) {
      sentinel.prev->next = &sentinel;
    }
    other.sentinel.next = other.sentinel.prev = &other.sentinel;
  }
  mutable list_element<Tag> sentinel;
};

template <typename T, typename Tag>
template <bool is_const>
struct list<T, Tag>::iterators_impl {
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = std::conditional_t<is_const, T const, T>;
  using pointer = std::conditional_t<is_const, T const*, T*>;
  using reference = std::conditional_t<is_const, T const&, T &>;


  template <bool is_arg_const=false, class=std::enable_if_t<is_const && !is_arg_const>>
  iterators_impl(iterators_impl<is_arg_const> const& other) : ptr(other.ptr) {}

  iterators_impl() : ptr(nullptr) {};

  pointer operator->() const {
    return static_cast<pointer>(ptr);
  }
  reference operator*() const {
    return static_cast<reference>(*ptr);
  }
  iterators_impl& operator++() & {
    ptr = static_cast<list_element<Tag>*>(ptr->next);
    return *this;
  }
  iterators_impl operator++(int) & {
    iterators_impl res(*this);
    ++(*this);
    return res;
  }
  iterators_impl& operator--() & {
    ptr = static_cast<list_element<Tag>*>(ptr->prev);
    return *this;
  }
  iterators_impl operator--(int) & {
    iterators_impl res(*this);
    --(*this);
    return res;
  }

  bool operator==(iterators_impl const& b) noexcept {
    return ptr == b.ptr;
  }

  bool operator!=(iterators_impl const& b) noexcept {
    return ptr != b.ptr;
  }

private:
  iterators_impl(list_element<Tag>* ptr) : ptr(ptr) {};
  list_element<Tag>* ptr;
  friend list;
};


} // namespace intrusive
