#pragma once

struct left_tag;
struct right_tag;

template <typename Tag>
struct base_node {
  base_node() noexcept : p(nullptr), l(nullptr), r(nullptr), height(1) {}

  base_node *p, *l, *r;
  uint16_t height;
};

struct base_binode : base_node<left_tag>, base_node<right_tag> {
  base_binode() noexcept = default;
};

template <typename T, typename Tag>
struct node : base_node<Tag> {
  explicit node(T&& val) : base_node<Tag>(), val(std::move(val)) {}
  explicit node(T const& val) : base_node<Tag>(), val(val) {}

  T val;
};


template <typename Left, typename Right>
struct binode : node<Left, left_tag>, node<Right, right_tag> {
  using l_node = node<Left, left_tag>;
  using r_node = node<Right, right_tag>;

  binode(Left const& lval, Right const& rval) : l_node(lval), r_node(rval) {}
  binode(Left const& lval, Right&& rval)
      : l_node(lval), r_node(std::move(rval)) {}
  binode(Left&& lval, Right const& rval)
      : l_node(std::move(lval)), r_node(rval) {}
  binode(Left&& lval, Right&& rval)
      : l_node(std::move(lval)), r_node(std::move(rval)) {}
  ~binode() = default;
};


template <typename T, typename CompT, typename Tag>
struct comparator : public CompT {
  using node_t = node<T, Tag>;

  comparator(CompT&& cmp_) : CompT(std::move(cmp_)) {}

  bool operator()(node_t const* a, node_t const* b) const {
    return CompT::operator()(a->val, b->val);
  }

  bool operator()(node_t const* a, T const& b) const {
    return CompT::operator()(a->val, b);
  }

  bool operator()(T const& a, node_t const* b) const {
    return CompT::operator()(a, b->val);
  }
};

template <typename T, typename CompT, typename Tag>
struct AVLTree {
  using node_t = base_node<Tag>;
  using val_node_t = node<T, Tag>;

  AVLTree() = delete;
  AVLTree(AVLTree const& other) = delete;
  AVLTree& operator=(AVLTree const& other) = delete;
  AVLTree(AVLTree&& other) = delete;
  AVLTree& operator=(AVLTree&& other) = delete;

  AVLTree(node_t* sentinel_, CompT&& cmp) : sentinel(sentinel_), cmp(std::move(cmp)) {};

  node_t* find(T const& val) const {
    node_t* res = find(sentinel->l, val);
    if (!res)
      return end();
    return res;
  }

  node_t* lower_bound(T const& val) const {
    node_t* res = lower_bound(sentinel->l, val);
    if (!res)
      return end();
    return res;
  }

  node_t* upper_bound(T const& val) const {
    node_t* res = upper_bound(sentinel->l, val);
    if (!res)
      return end();
    return res;
  }

  node_t* insert(node_t* new_node) {
    sentinel->l = insert(sentinel->l, new_node);
    update_children(sentinel);
    return new_node;
  }

  static node_t* erase(node_t* nd) {
    node_t* res = next(nd);
    unlink(nd);
    return res;
  }

  node_t* end() const noexcept {
    return sentinel;
  }

  node_t* begin() const noexcept {
    if (empty())
      return end();

    return find_min(sentinel->l);
  }

  node_t* end() noexcept {
    return sentinel;
  }

  node_t* begin() noexcept {
    if (empty())
      return end();
    return find_min(sentinel->l);
  }

  bool empty() const noexcept {
    return !sentinel || !sentinel->l;
  }

  void update_sentinel(node_t* new_sentinel) {
    sentinel = new_sentinel;
    update_children(new_sentinel);
  }

  static uint16_t height(node_t* nd) {
    if (!nd)
      return 0;
    return nd->height;
  }

  static void update_height(node_t* nd) noexcept {
    if (!nd)
      return;
    nd->height = std::max(height(nd->l), height(nd->r)) + 1;
  }

   static void update_children(node_t* nd) noexcept {
    if (!nd)
      return;
    if (nd->l) {
      nd->l->p = nd;
    }
    if (nd->r) {
      nd->r->p = nd;
    }
    update_height(nd);
  }

    static node_t* next(node_t* nd) noexcept {
    if (nd->r) {
      return find_min(nd->r);
    }
    while (!is_sentinel(nd->p) && nd->p->l != nd) {
      nd = nd->p;
    }
    return nd->p;
  }

  static node_t* prev(node_t* nd) noexcept {
    if (nd->l) {
      return find_max(nd->l);
    }
    while (!is_sentinel(nd->p) && nd->p->r != nd) {
      nd = nd->p;
    }
    return nd->p;
  }

  template <typename CastType>
  void remove_tree() {
    remove_tree<CastType>(sentinel->l);
  }

private:
  static int32_t balance_factor(node_t* nd) noexcept {
    if (!nd)
      return 0;
    return static_cast<int32_t>(height(nd->l)) -
           static_cast<int32_t>(height(nd->r));
  }

  static node_t* right_rotate(node_t* root) noexcept {
    node_t* rootl = root->l;
    root->l = rootl->r;
    rootl->r = root;

    update_children(root);
    update_children(rootl);

    return rootl;
  }

  static node_t* left_rotate(node_t* root) noexcept {
    node_t* rootr = root->r;
    root->r = rootr->l;
    rootr->l = root;

    update_children(root);
    update_children(rootr);

    return rootr;
  }

  static node_t* balance(node_t* nd) noexcept {
    update_children(nd);

    if (balance_factor(nd) == 2) {
      if (balance_factor(nd->l) < 0) {
        nd->l = left_rotate(nd->l);
      }
      return right_rotate(nd);
    }
    if (balance_factor(nd) == -2) {
      if (balance_factor(nd->r) > 0) {
        nd->r = right_rotate(nd->r);
      }
      return left_rotate(nd);
    }
    return nd;
  }

  static node_t* find_min(node_t* nd) noexcept {
    if (!nd)
      return nullptr;
    return nd->l ? find_min(nd->l) : nd;
  }

  static node_t* unlink_min(node_t* nd) noexcept {
    if (!nd)
      return nullptr;
    if (!nd->l) {
      if (is_left_child(nd)) {
        nd->p->l = nd->r;
      } else {
        nd->p->r = nd->r;
      }
      update_children(nd->p);
      return nd->r;
    }
    nd->l = unlink_min(nd->l);
    return balance(nd);
  }

  static node_t* find_max(node_t* nd) noexcept {
    if (!nd)
      return nullptr;
    return nd->r ? find_max(nd->r) : nd;
  }

  static void balance_up(node_t* nd) {
    if (is_sentinel(nd))
      return;
    node_t* parent = nd->p;
    if (is_left_child(nd)) {
      parent->l = balance(nd);
    } else {
      parent->r = balance(nd);
    }
    balance_up(parent);
  }


  static bool is_sentinel(node_t* nd) noexcept {
    return !nd || !nd->p;
  }

  node_t* find(node_t* root, T const& val) const {
    if (is_sentinel(root))
      return nullptr;
    if (static_cast<val_node_t*>(root)->val == val)
      return root;

    if (cmp(static_cast<val_node_t*>(root), val)) {
      return find(root->r, val);
    }
    return find(root->l, val);
  }

  node_t* lower_bound(node_t* root, T const& val) const {
    node_t* p = root->p;
    while (root) {
      if (cmp(static_cast<val_node_t*>(root), val)) {
        root = root->r;
      } else {
        p = root;
        root = root->l;
      }
    }
    return p;
  }

  node_t* upper_bound(node_t* root, T const& val) const {
    node_t* p = root->p;
    while (root) {
      if (!cmp(static_cast<val_node_t*>(root), val)) {
        p = root;
        root = root->l;
      } else {
        root = root->r;
      }
    }
    return p;
  }

  template <typename CastType>
  static void remove_tree(node_t* root) {
    if (!root)
      return;
    remove_tree<CastType>(root->l);
    remove_tree<CastType>(root->r);
    delete static_cast<CastType>(root);
  }

  node_t* insert(node_t* root, node_t* new_node) {
    if (!root)
      return new_node;

    if (cmp(static_cast<val_node_t*>(new_node), static_cast<val_node_t*>(root))) {
      root->l = insert(root->l, new_node);
    } else {
      root->r = insert(root->r, new_node);
    }
    return balance(root);
  }

  static node_t* prepare_unlink(node_t* nd) noexcept {
    if (!nd->l)
      return nd->r;
    if (!nd->r)
      return nd->l;
    node_t* mi = find_min(nd->r);
    mi->r = unlink_min(nd->r);
    mi->l = nd->l;
    return balance(mi);
  }

  static void unlink(node_t* nd) noexcept {
    if (is_left_child(nd)) {
      nd->p->l = prepare_unlink(nd);
    } else {
      nd->p->r = prepare_unlink(nd);
    }
    update_children(nd->p);
    balance_up(nd->p);
    nd->p = nd->l = nd->r = nullptr;
  }

  static bool is_left_child(node_t* nd) {
    if (!nd || !nd->p)
      return false;
    return nd->p->l == nd;
  }

  node_t* sentinel;
  CompT cmp;
};
