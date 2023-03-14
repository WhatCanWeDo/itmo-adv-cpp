#pragma once

#include "variant_exceptions.h"
#include "utils.h"

namespace get_ns {
template <typename Variadic_union>
constexpr decltype(auto) get_u(in_place_index_t<0>, Variadic_union&& v_union) {
    return std::forward<Variadic_union>(v_union).first.get();
}

template <size_t Ind, typename Variadic_union>
constexpr decltype(auto) get_u(in_place_index_t<Ind>, Variadic_union&& v_union) {
    return get_u(in_place_index<Ind-1>, std::forward<Variadic_union>(v_union).rest);
}

template <size_t Sizeof_types, size_t Ind, typename Variant>
constexpr decltype(auto) get_v(in_place_index_t<Ind>, Variant&& var) {
    if (!utils_ns::is_index_valid<Ind, Sizeof_types> || var.index() != Ind) {
        throw bad_variant_access();
    }
    return get_u(in_place_index<Ind>, std::forward<Variant>(var).storage);
}

} // get_ns

template <size_t Ind, typename Variant>
constexpr variant_alternative_t<Ind, Variant>& get(Variant& v) {
    return get_ns::get_v<variant_size_v<Variant>>(in_place_index<Ind>, v);
}

template <size_t Ind, typename Variant>
constexpr variant_alternative_t<Ind, Variant>&& get(Variant&& v) {
    return get_ns::get_v<variant_size_v<Variant>>(in_place_index<Ind>, std::move(v));
}

template <size_t Ind, typename Variant>
constexpr variant_alternative_t<Ind, Variant> const & get(Variant const& v) {
    return get_ns::get_v<variant_size_v<Variant>>(in_place_index<Ind>, v);
}

template <size_t Ind, typename Variant>
constexpr variant_alternative_t<Ind, Variant> const&& get(Variant const&& v) {
    return get_ns::get_v<variant_size_v<Variant>>(in_place_index<Ind>, std::move(v));
}

template <typename T, typename... Types>
constexpr T& get(variant<Types...>& v) {
    constexpr size_t ind = utils_ns::index_of_v<T, Types...>;
    return get_ns::get_v<sizeof...(Types)>(in_place_index<ind>, v);
}

template <typename T, typename... Types>
constexpr T&& get(variant<Types...>&& v) {
    constexpr size_t ind = utils_ns::index_of_v<T, Types...>;
    return get_ns::get_v<sizeof...(Types)>(in_place_index<ind>, std::move(v));
}

template <typename T, typename... Types>
constexpr T const& get(variant<Types...> const& v) {
    constexpr size_t ind = utils_ns::index_of_v<T, Types...>;
    return get_ns::get_v<sizeof...(Types)>(in_place_index<ind>, v);
}

template <typename T, typename... Types>
constexpr T const&& get(variant<Types...> const&& v) {
    constexpr size_t ind = utils_ns::index_of_v<T, Types...>;
    return get_ns::get_v<sizeof...(Types)>(in_place_index<ind>, v);
}

template <size_t Ind, typename... Types>
constexpr variant_alternative_t<Ind, variant<Types...>> * get_if(variant<Types...> * v) noexcept {
    if (!v || v->index() != Ind) return nullptr;
    return std::addressof(get<Ind>(*v));
}

template <size_t Ind, typename... Types>
constexpr variant_alternative_t<Ind, variant<Types...>> const* get_if(variant<Types...> const* v) noexcept {
    if (!v || v->index() != Ind) return nullptr;
    return std::addressof(get<Ind>(*v));
}


template <typename T, typename... Types>
constexpr T* get_if(variant<Types...> * v) noexcept {
    return get_if<utils_ns::index_of_v<T, Types...>, Types...>(v);
}

template <typename T, typename... Types>
constexpr T const* get_if(variant<Types...> const* v) noexcept {
    return get_if<utils_ns::index_of_v<T, Types...>, Types...>(v);
}
