#pragma once

#include <stdexcept>

namespace iprtl::proto {

class proto_exception : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

}