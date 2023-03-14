#include "intrusive_list.h"

namespace intrusive {
base_list_element::base_list_element() : prev(nullptr), next(nullptr){};
base_list_element::base_list_element(base_list_element&& other)
    : prev(other.prev), next(other.next) {
  other.prev = other.next = nullptr;
  if (prev) {
    prev->next = this;
  }
  if (next) {
    next->prev = this;
  }
}

base_list_element& base_list_element::operator=(base_list_element&& other) {
  if (other.next) {
    other.next->prev = this;
  }
  if (other.prev) {
    other.prev->next = this;
  }
  prev = std::move(other.prev);
  next = std::move(other.next);
  other.next = nullptr;
  other.prev = nullptr;
  return *this;
}

void base_list_element::unlink() {
  prev->next = next;
  next->prev = prev;
  prev = next = nullptr;
}

void base_list_element::insert_before(base_list_element* pos) {
  pos->prev->next = this;
  prev = pos->prev;
  next = pos;
  pos->prev = this;
}

bool base_list_element::in_list() const noexcept {
  return prev != nullptr;
}

base_list_element::~base_list_element() {
  if (in_list()) {
    unlink();
  }
}

} // namespace intrusive
