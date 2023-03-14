#pragma once

#include "variant_concepts.h"
#include "utils.h"

template <typename T>

struct wrapper {
    wrapper() = default;
    wrapper(wrapper const&) = default;
    wrapper(wrapper &&) = default;

    wrapper& operator=(wrapper const&) = default;
    wrapper& operator=(wrapper &&) = default;

    template <typename... Args>
    constexpr wrapper(Args &&... args) : val(std::forward<Args>(args)...) {}

    constexpr void swap(wrapper &other) {
        std::swap(val, other.val);
    }

    constexpr T& get()& noexcept {
        return val;
    }

    constexpr T const& get() const& noexcept {
        return val;
    }

    constexpr T&& get()&& noexcept {
        return std::move(val);
    }

    constexpr T const&& get() const&& noexcept {
        return std::move(val);
    }

    T val;
};

template <typename... Types>
union variadic_union;

template <TriviallyDestructible First, TriviallyDestructible... Rest>
union variadic_union<First, Rest...> {

    constexpr variadic_union() noexcept : rest() {}

    template<typename... Args>
	constexpr variadic_union(in_place_index_t<0>, Args &&... args)
	    : first(std::forward<Args>(args)...) {}

    template<size_t Ind, typename... Args>
	constexpr variadic_union(in_place_index_t<Ind>, Args &&... args)
	    : rest(in_place_index<Ind-1>, std::forward<Args>(args)...) {}

    template <size_t Ind>
    constexpr void destroy(in_place_index_t<Ind>) = delete;

    template <typename... Args>
    constexpr void emplace(in_place_index_t<0>, Args &&... args) {
        new (&first) wrapper<First>(std::forward<Args>(args)...);
    }

    template <size_t Ind, typename... Args>
    constexpr void emplace(in_place_index_t<Ind>, Args &&... args) {
        rest.emplace(in_place_index<Ind-1>, std::forward<Args>(args)...);
    }


    wrapper<First> first;
    variadic_union<Rest...> rest;
};

template <typename First, typename... Rest>
union variadic_union<First, Rest...> {

    constexpr variadic_union() noexcept : rest() {}

    template<typename... Args>
	constexpr variadic_union(in_place_index_t<0>, Args&&... args)
	    : first(std::forward<Args>(args)...) {}

    template<size_t Ind, typename... Args>
	constexpr variadic_union(in_place_index_t<Ind>, Args&&... args)
	    : rest(in_place_index<Ind-1>, std::forward<Args>(args)...) {}


    template <typename... Args>
    constexpr void emplace(in_place_index_t<0>, Args &&... args) {
        new (&first) wrapper<First>(std::forward<Args>(args)...);
    }

    template <size_t Ind, typename... Args>
    constexpr void emplace(in_place_index_t<Ind>, Args &&... args) {
        rest.emplace(in_place_index<Ind-1>, std::forward<Args>(args)...);
    }

    constexpr ~variadic_union() { } // don't forget to destruct instance (call destroy) inside owner-class

    wrapper<First> first;
    variadic_union<Rest...> rest;
};

template <>
union variadic_union<>{};
