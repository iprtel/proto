#pragma once

#include "set_field.h"
#include "types.h"
#include "field_access.h"
#include "concepts.h"
#include "iprtl/proto/wire/read.h"

namespace iprtl::proto::meta {
template<typename ProtoMessageT>
requires is_proto_message_p<ProtoMessageT>
inline std::span<const uint8_t> decode(std::span<const uint8_t> buffer,typename ProtoMessageT::cpp_type & message){
    while(not buffer.empty()) {
        auto [remaining, record] = iprtl::proto::wire::read_record(buffer);
        buffer=remaining;
        auto accessor = AccessorFactory<typename ProtoMessageT::message_fields>{}(record.field_number);
        if (accessor) {
            std::visit([&](auto&& access_tag) {
                using AccessorT = std::decay_t<decltype(access_tag)>;
                set_field<AccessorT>(message,record.value);
            }
            , *accessor);
        }
    }
    return buffer;
}
}