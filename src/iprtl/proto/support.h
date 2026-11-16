#pragma once

#include <cstdint>
#include <optional>
#include <variant>

namespace iprtl::proto {
template<typename AccessorVariant>
std::optional<AccessorVariant> AccessorSelector(uint32_t field) {
        return std::nullopt;
}

template<typename AccessorVariant, typename Accessor, typename ...Accessors>
std::optional<AccessorVariant> AccessorSelector(uint32_t field) {
    if (field == Accessor::number) {
        return AccessorVariant{Accessor{}};
    } else {
        return AccessorSelector<AccessorVariant, Accessors...>(field);
    }
};

template<typename AccessorVariant>
std::optional<AccessorVariant> IndexSelector(uint32_t index) {
        return std::nullopt;
}

template<typename AccessorVariant, typename Accessor, typename ...Accessors>
std::optional<AccessorVariant> IndexSelector(uint32_t index) {
    if (index == 0) {
        return AccessorVariant{Accessor{}};
    } else {
        return IndexSelector<AccessorVariant, Accessors...>(index - 1);
    }
};

template<typename T>
struct AccessorFactory;

template<typename ...Accessors>
struct AccessorFactory<std::variant<Accessors...>> {
    std::optional<std::variant<Accessors...>> operator()(uint32_t number) {
        return AccessorSelector<std::variant<Accessors...>, Accessors...>(number);
    }
    std::optional<std::variant<Accessors...>> byIndex(uint32_t index) {
        return IndexSelector<std::variant<Accessors...>, Accessors...>(index);
    }
};


}