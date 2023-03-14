#pragma once

#include <type_traits>

#include "variadic_union.h"
#include "visit.h"
#include "get.h"
#include "variant_exceptions.h"
#include "variant_base.h"


template <typename... Types>
struct variant : variant_destructor_base<(std::is_trivially_destructible_v<Types> && ...), Types...> {
    using base = variant_destructor_base<(std::is_trivially_destructible_v<Types> && ...), Types...>;

    template <size_t Ind, typename... Args>
    constexpr variant(in_place_index_t<Ind>, Args &&... args)
    requires ( utils_ns::is_index_valid<Ind, sizeof...(Types)> && std::is_constructible_v<utils_ns::Nth_type_t<Ind, Types...>, Args...> )
        : base(in_place_index<Ind>, std::forward<Args>(args)...) {}

    constexpr variant() noexcept(std::is_nothrow_default_constructible_v<utils_ns::Nth_type_t<0, Types...>>)
    requires std::is_default_constructible_v<utils_ns::Nth_type_t<0, Types...>>
        : base(in_place_index<0>) {}

    constexpr variant(variant const& other)
    requires AllTriviallyCopyConstructible<Types...> = default;

    constexpr variant(variant const& other)
    requires (AllCopyConstructible<Types...> && !AllTriviallyCopyConstructible<Types...>) : base() {
        if (!other.valueless_by_exception()) {
            visit_ns::visit_with_idxs<void>([this]<typename T>(T&& val, auto idx) {
                this->storage.emplace(in_place_index<idx>, std::forward<T>(val));
            }, other);
            this->_ind = other.index();
        }
    }


    constexpr variant(variant && other) noexcept( AllNothrowMoveConstructible<Types...> )
    requires AllTriviallyMoveConstructible<Types...>
        = default;

    constexpr variant(variant && other) noexcept( AllNothrowMoveConstructible<Types...> )
    requires (AllMoveConstructible<Types...>  && !AllTriviallyMoveConstructible<Types...>)
        : base() {
        if (!other.valueless_by_exception()) {
            visit_ns::visit_with_idxs<void>([this](auto&& val, auto idx) {
                this->storage.emplace(in_place_index<idx>, std::move(val));
            }, other);
            this->_ind = other.index();
        }
    }

    constexpr variant& operator=(variant const& other)
    requires ( AllTriviallyCopyConstructible<Types...> && AllTriviallyCopyAssignable<Types...> && AllTriviallyDestructible<Types...> )
        = default;

    constexpr variant& operator=(variant const& other)
    requires ( AllCopyConstructible<Types...> && AllCopyAssignable<Types...> &&
             ! (AllTriviallyCopyConstructible<Types...> && AllTriviallyCopyAssignable<Types...> && AllTriviallyDestructible<Types...>) ) {
        if (this == &other || (this->valueless_by_exception() && other.valueless_by_exception())) return *this;
        if (other.valueless_by_exception()) {
            this->reset();
            return *this;
        }
        visit_ns::visit_with_idxs<void>([this, &other](auto& this_value, auto const& other_value, auto this_index, auto other_index) {
            using other_value_type = utils_ns::Nth_type_t<other_index>;
            if constexpr (this_index == other_index) {
                this_value = other_value;
            } else if constexpr (std::is_nothrow_copy_constructible_v<other_value_type> || !std::is_nothrow_move_constructible_v<other_value_type>) {
                this->template emplace<other_index>(other_value);
            } else {
                this->operator=(variant(other));
            }
        }, *this, other);
        return *this;
    };

    constexpr variant& operator=(variant && other)
    noexcept(((std::is_nothrow_move_constructible_v<Types> && std::is_nothrow_move_assignable_v<Types>) && ...))
    requires ( AllTriviallyMoveConstructible<Types...> && AllTriviallyMoveAssignable<Types...> && AllTriviallyDestructible<Types...> )
        = default;

    constexpr variant& operator=(variant && other)
    noexcept(((std::is_nothrow_move_constructible_v<Types> && std::is_nothrow_move_assignable_v<Types>) && ...))
    requires ( AllMoveConstructible<Types...> && AllMoveAssignable<Types...> &&
             ! (AllTriviallyMoveConstructible<Types...> && AllTriviallyMoveAssignable<Types...> && AllTriviallyDestructible<Types...>) ) {
        if (this == &other || (this->valueless_by_exception() && other.valueless_by_exception())) return *this;
        if (other.valueless_by_exception()) {
            this->reset();
            return *this;
        }
        visit_ns::visit_with_idxs<void>([this](auto&& this_value, auto&& other_value, auto this_index, auto other_index){
            if constexpr (this_index == other_index) {
                this_value = std::move(other_value);
            } else {
                this->template emplace<other_index>(std::move(other_value));
            }
        }, *this, other);
        return *this;
    }


    template <class T, class... Args>
    constexpr T& emplace(Args&&... args)
    requires ( std::is_constructible_v<T, Args...> && utils_ns::exactly_once<T, Types...> ) {
        return emplace<utils_ns::index_of_v<T, Types...>>(std::forward<Args>(args)...);
    }

    template <size_t Ind, class... Args>
    constexpr variant_alternative_t<Ind, variant>& emplace(Args&&... args)
    requires ( std::is_constructible_v<utils_ns::Nth_type_t<Ind, Types...>, Args...> && Ind < sizeof...(Types) ) {
        if (!this->valueless_by_exception()) {
            this->reset();
        }
        this->storage.emplace(in_place_index<Ind>, std::forward<Args>(args)...);
        this->_ind = Ind;
        return get<Ind>(*this);
    }

    constexpr void swap(variant& other)
    noexcept (((std::is_nothrow_move_constructible_v<Types> && std::is_nothrow_swappable_v<Types>) && ...)) {
        if (this->valueless_by_exception() && other.valueless_by_exception()) return;
        if (this->valueless_by_exception()) {
            visit_ns::visit_with_idxs<void>([this](auto &&other_value, auto other_index){
                this->template emplace<other_index>(std::move(other_value));
            }, other);
            other.reset();
        }
        else if (other.valueless_by_exception()) {
            visit_ns::visit_with_idxs<void>([&other](auto &&this_value, auto this_index){
                other.template emplace<this_index>(std::move(this_value));
            }, *this);
            this->reset();
            return;
        }
        else {
            visit_ns::visit_with_idxs<void>([this, &other](auto& this_value, auto& other_value,  auto this_index, auto other_index){
                if constexpr (this_index == other_index) {
                    using std::swap;
                    swap(this_value, other_value);
                } else {
                    auto tmp = std::move(other_value);
                    other.template emplace<this_index>(std::move(this_value));
                    this->template emplace<other_index>(std::move(tmp));
                }
            }, *this, other);
        }
    }


    template <typename T, typename... Args>
    constexpr variant(in_place_type_t<T>, Args &&... args)
    requires ( utils_ns::exactly_once<T, Types...> && std::is_constructible_v<T, Args...> )
        : base(in_place_index<utils_ns::index_of_v<T, Types...>>, std::forward<Args>(args)...) {}

    template <typename T>
    constexpr variant(T&& t)
    noexcept (std::is_nothrow_constructible_v<utils_ns::matched_t<T, Types...>, T>)
    requires (
        sizeof...(Types) > 0
        && !std::is_same_v<utils_ns::matched_t<T, Types...>, void>
        && !std::is_same_v<std::remove_cvref_t<T>, variant>
        && std::is_constructible_v<utils_ns::matched_t<T, Types...>, T>
    ) : base(in_place_index<utils_ns::index_of_v<utils_ns::matched_t<T, Types...>, Types...>>, std::forward<T>(t)) {}

    template <typename T>
    constexpr variant& operator=(T && t)
    noexcept( (std::is_nothrow_assignable_v<utils_ns::matched_t<T, Types...>, T> && std::is_nothrow_constructible_v<utils_ns::matched_t<T, Types...>, T>) )
    requires (
        !std::is_same_v<utils_ns::matched_t<T, Types...>, void>
        && !std::is_same_v<std::remove_cvref_t<T>, variant>
        && std::is_assignable_v<utils_ns::matched_t<T, Types...>, T>
        && std::is_constructible_v<utils_ns::matched_t<T, Types...>, T>
    ) {
        using Matched_type = utils_ns::matched_t<T, Types...>;
        constexpr size_t Matched_index = utils_ns::index_of_v<Matched_type, Types...>;
        if (Matched_index == this->index()) {
            get<Matched_index>(*this) = std::forward<T>(t);
            return *this;
        }
        if constexpr (std::is_nothrow_constructible_v<Matched_type, T> || !std::is_nothrow_move_constructible_v<Matched_type>) {
            emplace<Matched_index>(std::forward<T>(t));
        } else {
            emplace<Matched_index>(Matched_type(std::forward<T>(t)));
        }
        return *this;
    }

    ~variant() = default;
};

template<typename... Types>
constexpr bool operator==(variant<Types...> const& v, variant<Types...> const& w) {
    if (v.index() != w.index()) return false;
    if (v.valueless_by_exception()) return true;
    return visit_ns::visit_with_idxs<bool>([&w](auto&& v_val, auto v_ind){
        return v_val == get<v_ind>(w);
    }, v);
}

template<typename... Types>
constexpr bool operator!=(variant<Types...> const& v, variant<Types...> const& w) {
    if (v.index() != w.index()) return true;
    if (v.valueless_by_exception()) return false;
    return visit_ns::visit_with_idxs<bool>([&w](auto&& v_val, auto v_ind){
        return v_val != get<v_ind>(w);
    }, v);
}

template<typename... Types>
constexpr bool operator<(variant<Types...> const& v, variant<Types...> const& w) {
    if (w.valueless_by_exception()) return false;
    if (v.valueless_by_exception()) return true;
    if (v.index() != w.index()) return v.index() < w.index();
    return visit_ns::visit_with_idxs<bool>([&w](auto&& v_val, auto v_ind){
        return v_val < get<v_ind>(w);
    }, v);
}

template<typename... Types>
constexpr bool operator>(variant<Types...> const& v, variant<Types...> const& w) {
    if (v.valueless_by_exception()) return false;
    if (w.valueless_by_exception()) return true;
    if (v.index() != w.index()) return v.index() > w.index();
    return visit_ns::visit_with_idxs<bool>([&w](auto&& v_val, auto v_ind){
        return v_val > get<v_ind>(w);
    }, v);
}

template<typename... Types >
constexpr bool operator<=(variant<Types...> const& v, variant<Types...> const& w) {
    if (v.valueless_by_exception()) return true;
    if (w.valueless_by_exception()) return false;
    if (v.index() != w.index()) return v.index() < w.index();
    return visit_ns::visit_with_idxs<bool>([&w](auto&& v_val, auto v_ind){
        return v_val <= get<v_ind>(w);
    }, v);
}

template<typename... Types>
constexpr bool operator>=(variant<Types...> const& v, variant<Types...> const& w) {
    if (w.valueless_by_exception()) return true;
    if (v.valueless_by_exception()) return false;
    if (v.index() != w.index()) return v.index() > w.index();
    return visit_ns::visit_with_idxs<bool>([&w](auto&& v_val, auto v_ind){
        return v_val >= get<v_ind>(w);
    }, v);
}


template<typename Type, typename... Types>
constexpr bool holds_alternative(variant<Types...> const& v) noexcept
requires utils_ns::exactly_once<Type, Types...> {
    return v.index() == utils_ns::index_of_v<Type, Types...>;
}
