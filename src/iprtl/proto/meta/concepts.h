#pragma once

#include <type_traits>
#include "iprtl/proto/concepts.h"
#include "types.h"

namespace iprtl::proto::meta {

template<typename ProtoT>
concept is_nested_p = is_instantiation_of<ProtoT, nested_p>::value;


template<typename>
struct is_proto_message : std::false_type {};

template<typename ClassT, FixedString Name, typename ...ProtoFields>
struct is_proto_message<ProtoMessage<ClassT, Name, ProtoFields...>> : std::true_type {};

template<typename ProtoT>
concept is_proto_message_p = is_proto_message<ProtoT>::value;


template<typename>
struct is_proto_field : std::false_type {};

template<typename ClassT, uint32_t Id, typename ProtoT, FixedString Name, typename ProtoT::cpp_type ClassT::* Member, typename V>
struct is_proto_field<ProtoField<ClassT, Id, ProtoT, Name, Member, V>> : std::true_type {};

template<typename ProtoT>
concept is_proto_field_p = is_proto_field<ProtoT>::value;

}
