#pragma once

#include <utility>

template<class... Ts> struct Overloaded: Ts... { using Ts::operator()...; };
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

template <class Variant, class... Cases>
constexpr auto match(Variant && variant, Cases &&... cases) {
    return std::visit(
        Overloaded { std::forward<Cases>(cases)... },
        std::forward<Variant>(variant)
    );
};
