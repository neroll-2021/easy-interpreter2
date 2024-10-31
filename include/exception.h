#ifndef NEROLL_SCRIPT_EXCEPTION_H
#define NEROLL_SCRIPT_EXCEPTION_H

#include <stdexcept>    // exception
#include <string>       // string
#include <format>       // format

namespace neroll {
    
namespace script {

class syntax_error : public std::exception {
 public:
    syntax_error(std::string msg)
        : message(std::move(msg)) {}

    [[nodiscard]]
    const char *what() const noexcept override {
        return message.c_str();
    }

 private:
    std::string message;
};

class execute_error : public std::exception {
 public:
    execute_error(std::string msg)
        : message(std::move(msg)) {}

    [[nodiscard]]
    const char *what() const noexcept override {
        return message.c_str();
    }
 private:
    std::string message;
};

class symbol_error : public std::exception {
 public:
    symbol_error(std::string msg)
        : message(std::move(msg)) {}

    [[nodiscard]]
    const char *what() const noexcept override {
        return message.c_str();
    }
 private:
    std::string message;
};

class type_error : public std::exception {
 public:
    type_error(std::string msg)
        : message(std::move(msg)) {}

    [[nodiscard]]
    const char *what() const noexcept override {
        return message.c_str();
    }
 private:
    std::string message;
};

template <typename T, typename... Args>
[[noreturn]]
void format_throw(const std::format_string<Args...> format_string, Args&&... args) {
    throw T(
        std::format(format_string, std::forward<Args>(args)...)
    );
}

template <typename... Args>
[[noreturn]]
void throw_syntax_error(std::format_string<Args...> format_string, Args&&... args) {
    format_throw<syntax_error>("[syntax error] {}",
        std::format(format_string, std::forward<Args>(args)...)
    );
}

template <typename... Args>
[[noreturn]]
void throw_execute_error(std::format_string<Args...> format_string, Args&&... args) {
    format_throw<execute_error>("[execute error] {}",
        std::format(format_string, std::forward<Args>(args)...)
    );
}

template <typename... Args>
[[noreturn]]
void throw_symbol_error(const std::format_string<Args...> format_string, Args&&... args) {
    format_throw<symbol_error>("[symbol error] {}",
        std::format(format_string, std::forward<Args>(args)...)
    );
}

template <typename... Args>
[[noreturn]]
void throw_type_error(std::format_string<Args...> format_string, Args&&... args) {
    format_throw<type_error>("[type error] {}",
        std::format(format_string, std::forward<Args>(args)...)
        // std::format(format_string, std::make_format_args(args...))
    );
}

}   // namespace script 

}   // namespace neroll

#endif