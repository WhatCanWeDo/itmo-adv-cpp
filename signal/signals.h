#pragma once
#include "intrusive_list.h"
#include <functional>
#include <unordered_map>

// Чтобы не было коллизий с UNIX-сигналами реализация вынесена в неймспейс, по
// той же причине изменено и название файла
namespace signals {

template <typename T>
struct signal;

template <typename... Args>
struct signal<void(Args...)> {
  using callback_t = std::function<void(Args...)>;
  struct connection_tag;

  struct connection : intrusive::list_element<connection_tag> {
    connection() noexcept : sig(nullptr){};

    connection(signal* sig, callback_t&& callback)
        : callback(std::move(callback)), sig(sig) {
      if (sig != nullptr) {
        sig->callbacks.push_back(*this);
      }
    }

    connection(connection&& other) : connection() {
      own_other_members(std::move(other));
    }

    connection& operator=(connection&& other) {
      if (&other == this) {
        return *this;
      }
      disconnect();
      own_other_members(std::move(other));
      return *this;
    }

    void disconnect() noexcept {
      if (!sig) {
        return;
      }
      for (auto cur = sig->tail; cur != nullptr; cur = cur->next) {
        if (cur->it != sig->callbacks.end() && &(*cur->it) == this) {
          ++cur->it;
        }
      }
      this->unlink();
      callback = {};
      sig = nullptr;
    }

    ~connection() {
      disconnect();
    }

  private:
    void own_other_members(connection&& other) {
      callback = std::move(other.callback);
      if (other.sig == nullptr) {
        sig = nullptr;
        return;
      }
      auto it = other.sig->callbacks.as_iterator(other);
      other.sig->callbacks.insert(++it, *this);
      sig = other.sig;
      other.disconnect();
    }

    callback_t callback;
    signal* sig;

    friend struct signal;
  };

  struct iteration_token {
    iteration_token(signal const* sig)
        : it(sig->callbacks.begin()), next(sig->tail), sig(sig) {
      sig->tail = this;
    }

    ~iteration_token() {
      if (sig) {
        sig->tail = next;
      }
    }

  private:
    friend struct signal;
    typename intrusive::list<connection, connection_tag>::const_iterator it;
    iteration_token* next;
    signal const* sig = nullptr;
  };

  signal() noexcept
      : callbacks(intrusive::list<connection, connection_tag>()),
        tail(nullptr) {}

  signal(signal const&) = delete;
  signal& operator=(signal const&) = delete;

  connection connect(callback_t slot) noexcept {
    return connection(this, std::move(slot));
  }

  ~signal() {
    for (iteration_token* token = tail; token != nullptr; token = token->next) {
      token->sig = nullptr;
    }
    while (!callbacks.empty()) {
      callbacks.back().disconnect();
    }
  }

  void operator()(Args... args) const {
    iteration_token token(this);
    while (token.sig && token.it != callbacks.end()) {
      auto cur = token.it;
      ++token.it;
      cur->callback(args...);
      if (token.sig == nullptr) {
        return;
      }
    }
  }

private:
  intrusive::list<connection, connection_tag> callbacks;
  mutable iteration_token* tail;
};

} // namespace signals
