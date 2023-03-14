#pragma once

#include "avl_tree.h"
#include <stdexcept>

template <typename Left, typename Right, typename CompareLeft = std::less<Left>,
          typename CompareRight = std::less<Right>>
struct bimap;

namespace details_ {

template <typename T, typename CompT, typename Tag, typename U, typename CompU,
          typename TagU>
struct iterator {
  using node_t = base_node<Tag>;
  using val_node_t = node<T, Tag>;
  using tree_t = AVLTree<T, CompT, Tag>;
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = T const*;
  using reference = T const&;

  iterator() noexcept : ptr(nullptr) {}
  iterator(iterator const& other) : ptr(other.ptr) {}

  reference operator*() const {
    return static_cast<val_node_t*>(ptr)->val;
  }

  pointer operator->() const {
    return &(static_cast<val_node_t*>(ptr)->val);
  }

  iterator& operator++() {
    ptr = tree_t::next(ptr);
    return *this;
  }

  iterator operator++(int) {
    iterator res = *this;
    ++(*this);
    return res;
  }

  iterator& operator--() {
    ptr = tree_t::prev(ptr);
    return *this;
  }

  iterator operator--(int) {
    iterator res = *this;
    --(*this);
    return res;
  }

  bool operator==(iterator const& other) const {
    return ptr == other.ptr;
  }

  bool operator!=(iterator const& other) const {
    return ptr != other.ptr;
  }

  iterator<U, CompU, TagU, T, CompT, Tag> flip() {
    if constexpr (std::is_same_v<Tag, left_tag>) {
      return iterator<U, CompU, TagU, T, CompT, Tag>(
          static_cast<binode<T, U>*>(ptr));
    } else {
      return iterator<U, CompU, TagU, T, CompT, Tag>(
          static_cast<binode<U, T>*>(ptr));
    }
  }

private:
  template <typename L, typename R, typename CL, typename CR>
  friend class ::bimap;

  template <typename L, typename CompL, typename TagL, typename R,
            typename CompR, typename TagR>
  friend class iterator;

  iterator(node_t* ptr_) : ptr(ptr_) {}
  node_t* ptr;
};
} // namespace details_

template <typename Left, typename Right, typename CompareLeft,
          typename CompareRight>
struct bimap {
  using left_t = Left;
  using right_t = Right;

  using node_t = binode<Left, Right>;
  using sentinel_t = base_binode;

  using l_cmp_t = comparator<Left, CompareLeft, left_tag>;
  using r_cmp_t = comparator<Right, CompareRight, right_tag>;

  using l_tree_t = AVLTree<Left, l_cmp_t, left_tag>;
  using r_tree_t = AVLTree<Right, r_cmp_t, right_tag>;

  using left_iterator =
      details_::iterator<Left, l_cmp_t, left_tag, Right, r_cmp_t, right_tag>;
  using right_iterator =
      details_::iterator<Right, r_cmp_t, right_tag, Left, l_cmp_t, left_tag>;

  bimap(CompareLeft compare_left = CompareLeft(),
        CompareRight compare_right = CompareRight())
      : tree_sentinel(),
        l_tree(&tree_sentinel, l_cmp_t(std::move(compare_left))),
        r_tree(&tree_sentinel, r_cmp_t(std::move(compare_right))), sz(0) {}

  bimap(bimap const& other) : bimap() {
    for (auto it = other.begin_left(); it != other.end_left(); ++it) {
      insert(*it, *it.flip());
    }
  }

  bimap(bimap&& other) noexcept : bimap() {
    swap(other);
  }

  bimap& operator=(bimap const& other) {
    bimap(other).swap(*this);
    return *this;
  }

  bimap& operator=(bimap&& other) noexcept {
    bimap(other).swap(*this);
    return *this;
  }

  ~bimap() {
    l_tree.template remove_tree<node_t*>();
  }

  left_iterator insert(left_t const& left, right_t const& right) {
    return insert_impl(left, right);
  }

  left_iterator insert(left_t const& left, right_t&& right) {
    return insert_impl(left, std::move(right));
  }

  left_iterator insert(left_t&& left, right_t const& right) {
    return insert_impl(std::move(left), right);
  }

  left_iterator insert(left_t&& left, right_t&& right) {
    return insert_impl(std::move(left), std::move(right));
  }

  left_iterator erase_left(left_iterator it) {
    --sz;
    auto bi_node = static_cast<node_t*>(it.ptr);
    auto l_node = l_tree_t::erase(bi_node);
    r_tree_t::erase(bi_node);
    delete bi_node;
    return left_iterator(l_node);
  }

  bool erase_left(left_t const& left) {
    if (left_iterator it = find_left(left); it != end_left()) {
      erase_left(it);
      return true;
    }
    return false;
  }

  right_iterator erase_right(right_iterator it) {
    --sz;
    auto bi_node = static_cast<node_t*>(it.ptr);
    l_tree_t::erase(bi_node);
    auto r_node = r_tree_t::erase(bi_node);
    delete bi_node;
    return right_iterator(r_node);
  }

  bool erase_right(right_t const& right) {
    if (right_iterator it = find_right(right); it != end_right()) {
      erase_right(it);
      return true;
    }
    return false;
  }

  left_iterator erase_left(left_iterator first, left_iterator last) {
    left_iterator res;
    left_iterator next = std::next(first);
    for (; first != last;
         first = next, next = (first == last ? first : std::next(first))) {
      res = erase_left(first);
    }
    return res;
  }

  right_iterator erase_right(right_iterator first, right_iterator last) {
    right_iterator res;
    right_iterator next = std::next(first);
    for (; first != last;
         first = next, next = (first == last ? first : std::next(first))) {
      res = erase_right(first);
    }
    return res;
  }

  left_iterator find_left(left_t const& left) const {
    return left_iterator(l_tree.find(left));
  }

  right_iterator find_right(right_t const& right) const {
    return right_iterator(r_tree.find(right));
  }

  right_t const& at_left(left_t const& key) const {
    auto left_it = find_left(key);
    if (left_it == end_left()) {
      throw std::out_of_range("bimap hasn't such element");
    }
    return *left_it.flip();
  }

  left_t const& at_right(right_t const& key) const {
    auto right_it = find_right(key);
    if (right_it == end_right()) {
      throw std::out_of_range("bimap hasn't such element");
    }
    return *right_it.flip();
  }

  template <typename = std::enable_if<std::is_default_constructible_v<right_t>>>
  right_t const& at_left_or_default(left_t const& key) {
    auto left_it = find_left(key);
    if (left_it == end_left()) {
      right_t default_right_element = right_t();
      auto right_it = find_right(default_right_element);
      if (right_it != end_right()) {
        erase_right(right_it);
      }
      auto res = insert(key, std::move(default_right_element));
      return *res.flip();
    } else {
      return *left_it.flip();
    }
  }

  template <typename = std::enable_if<std::is_default_constructible_v<left_t>>>
  left_t const& at_right_or_default(right_t const& key) {
    auto right_it = find_right(key);
    if (right_it == end_right()) {
      left_t default_left_element = left_t();
      auto left_it = find_left(default_left_element);
      if (left_it != end_left()) {
        erase_left(left_it);
      }
      auto res = insert(std::move(default_left_element), key);
      return *res;
    } else {
      return *right_it.flip();
    }
  }

  left_iterator lower_bound_left(const left_t& left) const {
    return left_iterator(l_tree.lower_bound(left));
  }

  left_iterator upper_bound_left(const left_t& left) const {
    return left_iterator(l_tree.upper_bound(left));
  }

  right_iterator lower_bound_right(const right_t& right) const {
    return right_iterator(r_tree.lower_bound(right));
  }

  right_iterator upper_bound_right(const right_t& right) const {
    return right_iterator(r_tree.upper_bound(right));
  }

  left_iterator begin_left() const {
    return left_iterator(l_tree.begin());
  }

  left_iterator end_left() const {
    return left_iterator(l_tree.end());
  }

  right_iterator begin_right() const {
    return right_iterator(r_tree.begin());
  }

  right_iterator end_right() const {
    return right_iterator(r_tree.end());
  }

  bool empty() const {
    return l_tree.empty();
  }

  std::size_t size() const {
    return sz;
  }

  void swap(bimap& other) {
    std::swap(tree_sentinel, other.tree_sentinel);
    std::swap(sz, other.sz);
    update_trees();
    other.update_trees();
  }

private:
  template <typename L, typename R>
  left_iterator insert_impl(L&& left, R&& right) {
    if (find_left(left) != end_left() || find_right(right) != end_right()) {
      return end_left();
    }
    node_t* nd = new node_t(std::forward<L>(left), std::forward<R>(right));
    ++sz;
    l_tree.insert(nd);
    r_tree.insert(nd);
    return left_iterator(nd);
  }

  void update_trees() noexcept {
    l_tree_t::update_children(&tree_sentinel);
    r_tree_t::update_children(&tree_sentinel);
  }

  template <typename lval, typename rval, typename lcmp, typename rcmp>
  friend bool operator==(bimap<lval, rval, lcmp, rcmp> const& a,
                         bimap<lval, rval, lcmp, rcmp> const& b);

  template <typename lval, typename rval, typename lcmp, typename rcmp>
  friend bool operator!=(bimap<lval, rval, lcmp, rcmp> const& a,
                         bimap<lval, rval, lcmp, rcmp> const& b);

  sentinel_t tree_sentinel;
  l_tree_t l_tree;
  r_tree_t r_tree;
  size_t sz;
};

template <typename Left, typename Right, typename CompareLeft,
          typename CompareRight>
bool operator==(bimap<Left, Right, CompareLeft, CompareRight> const& a,
                bimap<Left, Right, CompareLeft, CompareRight> const& b) {
  if (a.size() != b.size())
    return false;
  for (auto it_a = a.begin_left(), it_b = b.begin_left(); it_a != a.end_left();
       ++it_a, ++it_b) {
    if (*it_a != *it_b || *it_a.flip() != *it_b.flip()) {
      return false;
    }
  }
  return true;
}

template <typename Left, typename Right, typename CompareLeft,
          typename CompareRight>
bool operator!=(bimap<Left, Right, CompareLeft, CompareRight> const& a,
                bimap<Left, Right, CompareLeft, CompareRight> const& b) {
  return !(a == b);
}
