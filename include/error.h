#pragma once

#include <stdexcept>


struct AbstractError : std::runtime_error {
    AbstractError(const std::string& msg) : std::runtime_error("Error: " + msg + '.') {}
    AbstractError(const std::string& msg, int code) :
        std::runtime_error("Error: " + msg + ". (" + std::to_string(code) + ')')
    {}
};


template<typename RelatedType>
struct Error : AbstractError {
    Error(const std::string& msg) : AbstractError(msg) {}
    Error(const std::string& msg, int code) : AbstractError(msg, code) {}
};
