#pragma once

#include <exception>

struct bad_variant_access : std::exception {
    const char* what() const noexcept {
        return "bad variant access";
    }
};
