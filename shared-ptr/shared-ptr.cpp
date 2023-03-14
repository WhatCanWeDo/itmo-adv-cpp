#include "shared-ptr.h"
namespace shared_ptr_details {
control_block::control_block() noexcept = default;

void control_block::inc_strong() noexcept {
  strong_cnt += 1;
  weak_cnt += 1;
}

void control_block::inc_weak() noexcept {
  weak_cnt += 1;
}

void control_block::dec_weak() noexcept {
  weak_cnt -= 1;
  if (weak_cnt == 0) {
    delete this;
  }
}

void control_block::dec_strong() noexcept {
  strong_cnt -= 1;
  if (strong_cnt == 0) {
    delete_data();
  }
  dec_weak();
}

control_block::~control_block() = default;

size_t control_block::get_strong_cnt() const noexcept {
  return strong_cnt;
}

size_t control_block::get_weak_cnt() const noexcept {
  return weak_cnt;
}

}