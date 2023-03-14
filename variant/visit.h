#pragma once

#include "utils.h"
#include "get.h"
#include "variant_exceptions.h"

namespace visit_ns {
template <typename Type, size_t... Sizes>
struct MultiArray {
    constexpr decltype(auto) access() const noexcept {
        return data;
    }
    Type data;
};

template <typename Type, size_t Variant_size, size_t... Other_sizes>
struct MultiArray<Type, Variant_size, Other_sizes...> {

    template<typename... RestIndexes>
    constexpr decltype(auto) access(size_t first_index, RestIndexes... rest_indexes) const noexcept {
        return array[first_index].access(rest_indexes...);
    }

    MultiArray<Type, Other_sizes...> array[Variant_size];
};

template <bool pass_index_as_args, typename Array_type, typename Index_sequence>
struct gen_visit_table;

template <bool pass_index_as_args, typename R, typename Visitor, typename... Variants, size_t... Dimensions, size_t... Indexes>
struct gen_visit_table<pass_index_as_args, MultiArray<R (*)(Visitor, Variants...), Dimensions...>, std::index_sequence<Indexes...>> {
    using Array_type = MultiArray<R (*)(Visitor, Variants...), Dimensions...>;
    using Next = std::remove_cvref_t<utils_ns::Nth_type_t<sizeof...(Indexes), Variants...>>;

    static constexpr Array_type invoke() {
        Array_type v_table{};
        generate_v_table(v_table, std::make_index_sequence<variant_size_v<Next>>());
        return v_table;
    }

    template <size_t... Vars_indexes>
    static constexpr void generate_v_table(Array_type& v_table, std::index_sequence<Vars_indexes...>) {
        (generate_table_dimension<Vars_indexes>(v_table.array[Vars_indexes]), ...);
    }

    template<size_t Index, typename Arg_type>
    static constexpr void generate_table_dimension(Arg_type &arg) {
        arg = gen_visit_table<pass_index_as_args, std::remove_cvref_t<Arg_type>, std::index_sequence<Indexes..., Index>>::invoke();
    }
};

template <typename R, typename Visitor, typename... Variants, size_t... Indexes>
struct gen_visit_table<false, MultiArray<R (*)(Visitor, Variants...)>, std::index_sequence<Indexes...>> {
    using Array_type = MultiArray<R (*)(Visitor, Variants...)>;
    using Next = std::remove_cvref_t<utils_ns::Nth_type_t<sizeof...(Indexes), Variants...>>;

    static constexpr R apply_visitor(Visitor&& visitor, Variants&&... variants) {
        return std::forward<Visitor>(visitor)(get<Indexes>(std::forward<Variants>(variants))...);
    }

    static constexpr Array_type invoke() {
        return Array_type{&apply_visitor};
    }
};

template <typename R, typename Visitor, typename... Variants, size_t... Indexes>
struct gen_visit_table<true, MultiArray<R (*)(Visitor, Variants...)>, std::index_sequence<Indexes...>> {
    using Array_type = MultiArray<R (*)(Visitor, Variants...)>;

    static constexpr R apply_visitor(Visitor&& visitor, Variants&&... variants) {
        return std::forward<Visitor>(visitor)(get<Indexes>(std::forward<Variants>(variants))..., std::integral_constant<size_t, Indexes>()...);
    }

    static constexpr Array_type invoke() {
        return Array_type{&apply_visitor};
    }
};


template <bool with_idxs, typename R, typename Visitor, typename... Variants>
struct table_wrapper {
    using table_t = MultiArray<R (*)(Visitor &&, Variants &&...), variant_size_v<std::remove_cvref_t<Variants>>...>;
    static constexpr auto table = gen_visit_table<with_idxs, table_t, std::index_sequence<>>::invoke();
};

template <typename R, typename Visitor, typename... Variants>
constexpr R visit_with_idxs(Visitor&& visitor, Variants &&... variants) {
    if ((variants.valueless_by_exception() || ...)) {
        throw bad_variant_access();
    }
    using wrapper = table_wrapper<true, R, Visitor, Variants...>;
    auto func = wrapper::table.access(variants.index()...);
    return (*func)(std::forward<Visitor>(visitor), std::forward<Variants>(variants)...);
}

template <typename R, typename Visitor, typename... Variants>
constexpr R visit_impl(Visitor&& visitor, Variants &&... variants) {
    if ((variants.valueless_by_exception() || ...)) {
        throw bad_variant_access();
    }
    using wrapper = table_wrapper<false, R, Visitor, Variants...>;
    auto func = wrapper::table.access(variants.index()...);
    return (*func)(std::forward<Visitor>(visitor), std::forward<Variants>(variants)...);
}

} // visit_ns

template <typename R, typename Visitor, typename... Variants>
constexpr R visit(Visitor&& visitor, Variants &&... variants) {
    return visit_ns::visit_impl<R>(std::forward<Visitor>(visitor), std::forward<Variants>(variants)...);
}

template <typename Visitor, typename... Variants>
constexpr decltype(auto) visit(Visitor&& visitor, Variants &&... variants) {
    using R = decltype(std::forward<Visitor>(visitor)(get<0>(std::forward<Variants>(variants))...));
    return visit<R>(std::forward<Visitor>(visitor), std::forward<Variants>(variants)...);
}
