#pragma once
#include <stdexcept>

namespace X123Driver {

class GenericException : public std::runtime_error {
public:
    GenericException(const std::string& what) :
        std::runtime_error(what)
    { }
    ~GenericException()
    { }
};
}
