#pragma once

#include "types.h"
#include "iprtl/proto/proto_exception.h"
#include "concepts.h"
#include "as.h"
#include "iprtl/proto/packed_repeated_values.h"

namespace iprtl::proto::meta {

template <typename MetaField, typename ObjectT>
requires (! is_repeated_p<typename MetaField::proto_type>)
void set_field(ObjectT& object, typename MetaField::wire_type const& value) {
    auto v =  typename MetaField::member_type{as<typename MetaField::proto_type>(value)};
    MetaField::access(object) = v;
};

template <typename MetaField, typename ObjectT>
requires (! is_repeated_field<typename MetaField::proto_type>)
void set_field(ObjectT& object, wire::wire_value const& value) {
    fmt::print("set_field: {}={} {}\n", std::string_view{MetaField::name.data(),MetaField::name.size()}, value, MetaField::wire_type::name);
    if (not std::holds_alternative<typename MetaField::wire_type>(value)) throw wire::wire_exception(MetaField::name.data());
    auto wire_value =  std::get<typename MetaField::wire_type>(value);
    auto v =  typename MetaField::member_type{as<typename MetaField::proto_type>(wire_value)};
    MetaField::access(object) = v;
    fmt::print("set_field: {}={}\n", std::string_view{MetaField::name.data(),MetaField::name.size()}, MetaField::access(object));
};

template <typename MetaField, typename ObjectT>
requires is_repeated_p<typename MetaField::proto_type>
void set_field(ObjectT& object, wire::wire_value const& value) {
    if (std::holds_alternative<typename MetaField::wire_type>(value)){
        auto wire_value =  std::get<typename MetaField::wire_type>(value);
        MetaField::access(object).push_back(as<typename MetaField::proto_type::element_type>(wire_value));
    }
};

template <typename MetaField, typename ObjectT>
requires is_packed_p<typename MetaField::proto_type>
void set_field(ObjectT& object, wire::wire_value const& value) {
    // Packed repeated fields are encoded as length-delimited, so we expect a len_data wire type
    auto wire_value =  std::get<wire::len_data>(value);
    packed_repeated_values<typename MetaField::proto_type::element_type> packaged;
    packaged.data = wire_value;
    for (auto v : packaged) {
        MetaField::access(object).push_back(v);
    }
};


template <typename MetaField, typename ObjectT>
requires is_repeated_p<typename MetaField::proto_type>
void set_field(ObjectT& object, typename MetaField::wire_type const& value) {
    MetaField::access(object).push_back(as<typename MetaField::proto_type::element_type>(value));
};

template <typename MetaField, typename ObjectT>
requires is_packed_repeated<typename MetaField::proto_type> || is_packed_p<typename MetaField::proto_type>
void set_field(ObjectT& object, wire::len_data const& value) {
    packed_repeated_values<typename MetaField::proto_type::element_type::cpp_type> packaged{};
    packaged.data = value;
    for (auto v : packaged) {
        MetaField::access(object).push_back(v);
    }
};
}