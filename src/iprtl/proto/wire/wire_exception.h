#pragma once

#include <stdexcept>

namespace iprtl::proto {

class wire_exception : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

}