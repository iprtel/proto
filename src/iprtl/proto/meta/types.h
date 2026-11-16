#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <variant>

#include "fixedstring.h"
#include "iprtl/proto/wire/types.h"

namespace iprtl::proto::meta {

template <typename ClassT, uint32_t Id, typename ProtoT, FixedString Name, typename ProtoT::cpp_type ClassT::* Member,typename = void>
struct ProtoField {
  static constexpr uint32_t number {Id};
  static constexpr std::array<char,Name.size> name {Name.value};
  using class_type = ClassT;
  using cpp_type = ProtoT::cpp_type;
  using member_type = ProtoT::cpp_type;
  using member_declaration = member_type ClassT::*;
  using proto_type = ProtoT;
  using wire_type = ProtoT::wire_type;
  inline static constexpr decltype(Member) member = Member;
  template<typename ObjectT>
  static member_type& access(ObjectT& object) {
    return std::invoke(member, std::forward<ObjectT &>(object));
  }
  template<typename ObjectT>
  static member_type const& access(ObjectT const& object) {
    return std::invoke(member, std::forward<ObjectT const&>(object));
  }
};

template <typename ClassT, FixedString Name, typename ...ProtoFields>
struct ProtoMessage {
  static constexpr std::array<char,Name.size> name {Name.value};
  using cpp_type = ClassT;
  using message_type = ClassT;
  using message_fields = std::variant<ProtoFields...>;
};

// A field whose value is another message (length-delimited on the wire).
template<typename T>
struct nested_p {
    using nested_type = T;
    using cpp_type = typename T::cpp_type;
    using wire_type = wire::len_data;
};
}
