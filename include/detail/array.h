#ifndef NEROLL_SCRIPT_DETAIL_ARRAY_H
#define NEROLL_SCRIPT_DETAIL_ARRAY_H

#include <cstdint>  // int32_t
#include <variant>  // variant
#include <string>   // string
#include <vector>   // vector
#include <memory>   // shared_ptr
#include "variable.h"

namespace neroll {

namespace script {

namespace detail {

class array;
using value_t = std::variant<int32_t, double, bool, std::string, char, array>;

class array {
 public:
    using size_type = std::vector<value_t>::size_type;

    array(variable_type type)
        : elem_type_(type), data_(std::make_shared<std::vector<value_t>>()) {}

    [[nodiscard]]
    size_type size() const noexcept {
        return data_->size();
    }

    [[nodiscard]]
    bool empty() const noexcept {
        return data_->empty();
    }

    [[nodiscard]]
    bool is_null() const noexcept {
        return data_ == nullptr;
    }

    [[nodiscard]]
    variable_type elem_type() const noexcept {
        return elem_type_;
    }

    void push_back(value_t value) {
        data_->emplace_back(std::move(value));
    }

    const value_t &operator[](std::size_t index) const noexcept {
        return data_->operator[](index);
    }
    value_t &operator[](std::size_t index) noexcept {
        return data_->operator[](index);
    }

 private:
    std::shared_ptr<std::vector<value_t>> data_;
    variable_type elem_type_;
};

}

}

}

#endif