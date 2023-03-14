#pragma once

#include "variadic_union.h"
#include "utils.h"
#include <type_traits>
#include "visit.h"

template <bool is_trivially_destructible, typename... Types>
struct variant_destructor_base {
    constexpr variant_destructor_base() noexcept : storage(), _ind(variant_npos) {}

    template <size_t Ind, typename... Args>
    constexpr variant_destructor_base(in_place_index_t<Ind>, Args &&... args)
    requires utils_ns::is_index_valid<Ind, sizeof...(Types)> && std::is_constructible_v<utils_ns::Nth_type_t<Ind, Types...>, Args...>
        : storage(in_place_index<Ind>, std::forward<Args>(args)...), _ind(Ind) {}

    constexpr void reset() {
        _ind = variant_npos;
    }

    constexpr bool valueless_by_exception() const noexcept {
        return _ind == variant_npos;
    }

    constexpr size_t index() const noexcept {
        return _ind;
    }

protected:
    variadic_union<Types...> storage;
    size_t _ind = 0;

    template <size_t SZT, size_t I, typename V>
    friend constexpr decltype(auto) get_ns::get_v(in_place_index_t<I>, V&&);
};

template <typename... Types>
struct variant_destructor_base<false, Types...> {
    constexpr variant_destructor_base() noexcept : storage(), _ind(variant_npos) {}

    template <size_t Ind, typename... Args>
    constexpr variant_destructor_base(in_place_index_t<Ind>, Args &&... args)
    requires utils_ns::is_index_valid<Ind, sizeof...(Types)> && std::is_constructible_v<utils_ns::Nth_type_t<Ind, Types...>, Args...>
        : storage(in_place_index<Ind>, std::forward<Args>(args)...), _ind(Ind) {}

    constexpr void reset() {
        if (_ind == variant_npos) return;
        visit_ns::visit_impl<void>(
            [](auto&& val) {
                using T = std::remove_cvref_t<decltype(val)>;
                val.~T();
            },
            *this
        );
        _ind = variant_npos;
    }

    ~variant_destructor_base() {
        reset();
    }

    constexpr bool valueless_by_exception() const noexcept {
        return _ind == variant_npos;
    }

    constexpr size_t index() const noexcept {
        return _ind;
    }

protected:
    variadic_union<Types...> storage;
    size_t _ind;

    template <size_t SZT, size_t I, typename V>
    friend constexpr decltype(auto) get_ns::get_v(in_place_index_t<I>, V&&);
};
