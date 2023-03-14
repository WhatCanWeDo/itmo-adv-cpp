#pragma once

#include "variadic_union.h"
#include <type_traits>


template <bool trivial_destr, typename... Types>
struct variant_destructor_base;

template <typename... Types>
struct variant;

namespace utils_ns {
template <size_t N, typename... Rest>
struct Nth_type {
    using type = void;
};

template <size_t N, typename Type, typename... Rest>
struct Nth_type<N, Type, Rest...> {
    using type = typename Nth_type<N-1, Rest...>::type;
};

template <typename Type, typename... Rest>
struct Nth_type<0, Type, Rest...> {
    using type = Type;
};

template <size_t N, typename... Types>
using Nth_type_t = typename Nth_type<N, Types...>::type;


template <size_t Ind, size_t MaxInd>
constexpr bool is_index_valid = Ind < MaxInd;


template <typename Target, typename... Types>
struct index_of : std::integral_constant<size_t, 0> {};

template <typename Target, typename... Types>
inline constexpr size_t index_of_v = index_of<Target, Types...>::value;

template <typename Target, typename Type, typename... Rest>
struct index_of<Target, Type, Rest...>
    : std::integral_constant<size_t, std::is_same_v<Target, Type> ? 0 : index_of_v<Target, Rest...> + 1> {};


template <typename Target, typename... Types>
struct count_type : std::integral_constant<size_t, 0> {};

template <typename Target, typename... Types>
inline constexpr size_t count_type_v = count_type<Target, Types...>::value;

template <typename Target, typename Type, typename... Rest>
struct count_type <Target, Type, Rest...>
    : std::integral_constant<size_t, std::is_same_v<Target, Type> + count_type_v<Target, Rest...>> {};

template <typename Target, typename... Types>
inline constexpr bool exactly_once = (count_type_v<Target, Types...> == 1);


// helps check if conversion possible

template <typename T>
struct array {
    T a[1];
};

template <typename Target, typename Type>
concept Convertible = requires(Target&& t) {
    array<Type>{{ std::forward<Target>(t) }};
};

template <typename Target, typename Type>
concept convertible_counting_bool = Convertible<Target, Type>
    && (!std::is_same_v<std::remove_cvref_t<Type>, bool> || std::is_same_v<std::remove_cvref_t<Target>, bool> );

template <typename Target, typename Index_seq, typename... Types>
struct convertible;

template <typename Target, size_t... Index_seq, typename... Types>
struct convertible<Target, std::index_sequence<Index_seq...>, Types...> {
    using type = void;
};

template <typename Target, size_t Index, size_t... Index_seq, typename Type, typename... Types>
requires convertible_counting_bool<Target, Type>
struct convertible<Target, std::index_sequence<Index, Index_seq...>, Type, Types...> {
    using type = Type;
};

template <typename Target, size_t Index, size_t... Index_seq, typename Type, typename... Types>
requires ( !convertible_counting_bool<Target, Type> )
struct convertible<Target, std::index_sequence<Index, Index_seq...>, Type, Types...> {
    using type = typename convertible<Target, std::index_sequence<Index_seq...>, Types...>::type;
};

template <typename Target, typename... Types>
using convertible_t = typename convertible<Target, std::make_index_sequence<sizeof...(Types)>, Types...>::type;


template <typename Target, typename... Types>
struct matched;

template <typename Target, typename... Types>
requires ( count_type_v<Target, Types...> > 0 )
struct matched<Target, Types...> {
    using type = Target;
};

template <typename Target, typename... Types>
requires ( count_type_v<Target, Types...> == 0 )
struct matched<Target, Types...> {
    using type = convertible_t<Target, Types...>;
};

template <typename Target, typename... Types>
using matched_t = typename matched<Target, Types...>::type;
} // utils_ns

template <typename Variant>
struct variant_size;


template <typename... Types>
struct variant_size<variant<Types...>> : std::integral_constant<size_t, sizeof...(Types)> {};

template <typename... Types>
struct variant_size<variant_destructor_base<true, Types...>> : std::integral_constant<size_t, sizeof...(Types)> {};

template <typename... Types>
struct variant_size<variant_destructor_base<false, Types...>> : std::integral_constant<size_t, sizeof...(Types)> {};

template <typename Variant>
struct variant_size<const Variant> : variant_size<Variant> {};

template <typename Variant>
inline constexpr size_t variant_size_v = variant_size<Variant>::value;


template<size_t Ind, typename Variant>
struct variant_alternative;

template<size_t Ind, typename First, typename... Rest>
struct variant_alternative<Ind, variant<First, Rest...>> {
    using type = typename variant_alternative<Ind-1, variant<Rest...>>::type;
};

template<typename First, typename... Rest>
struct variant_alternative<0, variant<First, Rest...>> {
    using type = First;
};

template<size_t Ind, typename First, typename... Rest>
struct variant_alternative<Ind, variant_destructor_base<false, First, Rest...>> {
    using type = typename variant_alternative<Ind-1, variant<Rest...>>::type;
};

template<typename First, typename... Rest>
struct variant_alternative<0, variant_destructor_base<false, First, Rest...>> {
    using type = First;
};

template<size_t Ind, typename First, typename... Rest>
struct variant_alternative<Ind, variant_destructor_base<true, First, Rest...>> {
    using type = typename variant_alternative<Ind-1, variant<Rest...>>::type;
};

template<typename First, typename... Rest>
struct variant_alternative<0, variant_destructor_base<true, First, Rest...>> {
    using type = First;
};

template<size_t Ind, typename Variant>
using variant_alternative_t = typename variant_alternative<Ind, Variant>::type;

template<size_t Ind, typename Variant>
struct variant_alternative<Ind, const Variant> {
    using type = typename std::add_const_t<variant_alternative_t<Ind, Variant>>;
};

template<typename Type> struct in_place_type_t {
    explicit in_place_type_t() = default;
};

template<typename Type>
inline constexpr in_place_type_t<Type> in_place_type{};

template<size_t Ind>
struct in_place_index_t {
    explicit in_place_index_t() = default;
};

template<size_t Ind>
inline constexpr in_place_index_t<Ind> in_place_index{};

inline constexpr size_t variant_npos = -1;
