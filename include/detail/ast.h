#ifndef NEROLL_SCRIPT_DETAIL_AST_H
#define NEROLL_SCRIPT_DETAIL_AST_H

#include <cstdint>
#include <limits>
#include <optional> // optional
#include <variant>  // variant, get
#include <string>   // string
#include <memory>   // shared_ptr
#include <concepts> // is_same_v
#include <cassert>  // assert
#include <utility>  // pair

#include "array.h"
#include "variable.h"
#include "exception.h"
#include "operator.h"

namespace neroll {

namespace script {

namespace detail {

// only for test
template <typename T>
constexpr static void static_type_check() {
    static_assert(std::is_same_v<T, int32_t> ||
        std::is_same_v<T, double> || std::is_same_v<T, bool> ||
        std::is_same_v<T, std::string> || std::is_same_v<T, char> ||
        std::is_same_v<T, array>);
}

class ast_node {
 public:
    virtual ~ast_node() = default;
};

class expr_node : public ast_node {
 public:
    // using value_t = neroll::script::detail::value_t;
    // expr_node(value_t value) : value_(std::move(value)) {}

    void set_value(value_t value) {
        value_ = std::move(value);
    }

    [[nodiscard]]
    const value_t &value() const noexcept {
        return value_;
    }
    value_t &value() noexcept {
        return value_;
    }

    template <typename T>
    auto &get() {
        static_type_check<T>();
        return std::get<T>(value_);
    }

    template <typename T>
    [[nodiscard]]
    const auto &get() const {
        static_type_check<T>();
        return std::get<T>(value_);
    }

    virtual void evaluate() = 0;

    [[nodiscard]]
    variable_type eval_type() const {
        return static_cast<variable_type>(value_.index());
    }

 private:
    value_t value_;
};

class binary_expr_node : public expr_node {
 public:
    binary_expr_node(
        std::shared_ptr<expr_node> lhs, std::shared_ptr<expr_node> rhs
    ) : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    virtual ~binary_expr_node() = default;

    [[nodiscard]]
    std::shared_ptr<expr_node> lhs() const noexcept {
        return lhs_;
    }

    [[nodiscard]]
    std::shared_ptr<expr_node> rhs() const noexcept {
        return rhs_;
    }

    template <typename T, typename U, typename Op>
    void operation() {
        static_assert(!std::is_same_v<T, array>);
        static_assert(!std::is_same_v<U, array>);
        const T &lhs = lhs_->get<T>();
        const U &rhs = rhs_->get<U>();
        set_value(Op{}(lhs, rhs));
    }

 private:
    std::shared_ptr<expr_node> lhs_;
    std::shared_ptr<expr_node> rhs_;
};

class binary_arithmetic_node : public binary_expr_node {
 public:
    binary_arithmetic_node(std::shared_ptr<expr_node> lhs , std::shared_ptr<expr_node> rhs)
        : binary_expr_node(std::move(lhs), std::move(rhs)) {}
    
    template <typename Operator>
    void int_int() {
        operation<int, int, Operator>();
    }

    template <typename Operator>
    void int_float() {
        operation<int, double, Operator>();
    }

    template <typename Operator>
    void float_int() {
        operation<double, int, Operator>();
    }

    template <typename Operator>
    void float_float() {
        operation<double, double, Operator>();
    }

    template <typename Operator>
    void string_string() {
        operation<std::string, std::string, Operator>();
    }

};

class add_node final : public binary_arithmetic_node {
 public:
    add_node(std::shared_ptr<expr_node> left , std::shared_ptr<expr_node> right)
        : binary_arithmetic_node(std::move(left), std::move(right)) {
        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        if (is_both_string(lhs_type, rhs_type)) {
            return;
        }

        variable_type type = arithmetic_type_cast(lhs_type, rhs_type);

        if (arithmetic_type_cast(lhs_type, rhs_type) == variable_type::error) {
            throw_type_error("invalid operator + between {} and {}", lhs_type, rhs_type);
        }

        switch (type) {
            case variable_type::integer:
                set_value(int32_t{});
                break;
            case variable_type::floating:
                set_value(double{});
                break;
            case variable_type::boolean:
                set_value(bool{});
                break;
            case variable_type::string:
                set_value(std::string{});
                break;
            case variable_type::character:
                set_value(char{});
                break;
            default:
                std::unreachable();
        }
    }

    void evaluate() override {
        lhs()->evaluate();
        rhs()->evaluate();

        using a = binary_arithmetic_node;

        // cannot be constexpr because MSVC will make pointers incorrect
        const static std::array<std::array<void(a::*)(), 5>, 5> table {{
             /*      int                  float            boolean         string               char   */
/* int    */{ &a::int_int<plus>,    &a::int_float<plus>,   nullptr,        nullptr,            nullptr },
/* float  */{&a::float_int<plus>,  &a::float_float<plus>,  nullptr,        nullptr,            nullptr },
/* bool   */{     nullptr,               nullptr,          nullptr,        nullptr,            nullptr },
/* string */{     nullptr,               nullptr,          nullptr,  &a::string_string<plus>,  nullptr },
/* char   */{     nullptr,               nullptr,          nullptr,        nullptr,            nullptr }
        }};

        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        assert(lhs_type != variable_type::error);
        assert(rhs_type != variable_type::error);

        auto lhs_index = static_cast<std::size_t>(lhs_type);
        auto rhs_index = static_cast<std::size_t>(rhs_type);

        // assert(table[0][0] == &a::int_int<plus>);
        // assert(table[0][1] == &a::int_float<plus>);
        // assert(table[1][0] == &a::float_int<plus>);
        // assert(table[1][0] != nullptr);
        // assert(table[1][1] == &a::float_float<plus>);
        // assert(table[3][2] == nullptr);
        // assert(table[3][3] == &a::string_string<plus>);

        (this->*table[lhs_index][rhs_index])();
    }
};

class minus_node final : public binary_arithmetic_node {
 public:
    minus_node(std::shared_ptr<expr_node> left , std::shared_ptr<expr_node> right)
        : binary_arithmetic_node(std::move(left), std::move(right)) {
        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        if (arithmetic_type_cast(lhs_type, rhs_type) == variable_type::error) {
            throw_type_error("invalid operator - between {} and {}", lhs_type, rhs_type);
        }
    }

    void evaluate() override {
        lhs()->evaluate();
        rhs()->evaluate();

        using a = binary_arithmetic_node;

        const static std::array<std::array<void(binary_arithmetic_node::*)(), 2>, 2> table {{
             /*        int                    float         */
/* int    */{  &a::int_int<minus>,     &a::int_float<minus>  },
/* float  */{ &a::float_int<minus>,   &a::float_float<minus> },
        }};

        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        assert(lhs_type != variable_type::error);
        assert(rhs_type != variable_type::error);

        auto lhs_index = static_cast<std::size_t>(lhs_type);
        auto rhs_index = static_cast<std::size_t>(rhs_type);
        
        (this->*table[lhs_index][rhs_index])();
    }
};

class multiply_node final : public binary_arithmetic_node {
 public:
    multiply_node(std::shared_ptr<expr_node> left , std::shared_ptr<expr_node> right)
        : binary_arithmetic_node(std::move(left), std::move(right)) {
        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        variable_type type = arithmetic_type_cast(lhs_type, rhs_type);

        if (arithmetic_type_cast(lhs_type, rhs_type) == variable_type::error) {
            throw_type_error("invalid operator * between {} and {}", lhs_type, rhs_type);
        }

        switch (type) {
            case variable_type::integer:
                set_value(int32_t{});
                break;
            case variable_type::floating:
                set_value(double{});
                break;
            case variable_type::boolean:
                set_value(bool{});
                break;
            case variable_type::string:
                set_value(std::string{});
                break;
            case variable_type::character:
                set_value(char{});
                break;
            default:
                std::unreachable();
        }
    }

    void evaluate() override {
        lhs()->evaluate();
        rhs()->evaluate();

        using a = binary_arithmetic_node;

        const static std::array<std::array<void(binary_arithmetic_node::*)(), 2>, 2> table {{
             /*            int                       float          */
/* int    */{  &a::int_int<multiplies>,   &a::int_float<multiplies>  },
/* float  */{ &a::float_int<multiplies>, &a::float_float<multiplies> },
        }};

        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        assert(lhs_type != variable_type::error);
        assert(rhs_type != variable_type::error);

        auto lhs_index = static_cast<std::size_t>(lhs_type);
        auto rhs_index = static_cast<std::size_t>(rhs_type);
        
        (this->*table[lhs_index][rhs_index])();
    }
};


class divide_node final : public binary_arithmetic_node {
 public:
    divide_node(std::shared_ptr<expr_node> left , std::shared_ptr<expr_node> right)
        : binary_arithmetic_node(std::move(left), std::move(right)) {
        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        variable_type type = arithmetic_type_cast(lhs_type, rhs_type);

        if (arithmetic_type_cast(lhs_type, rhs_type) == variable_type::error) {
            throw_type_error("invalid operator / between {} and {}", lhs_type, rhs_type);
        }

        switch (type) {
            case variable_type::integer:
                set_value(int32_t{});
                break;
            case variable_type::floating:
                set_value(double{});
                break;
            case variable_type::boolean:
                set_value(bool{});
                break;
            case variable_type::string:
                set_value(std::string{});
                break;
            case variable_type::character:
                set_value(char{});
                break;
            default:
                std::unreachable();
        }
    }

    void evaluate() override {
        lhs()->evaluate();
        rhs()->evaluate();

        if (rhs()->eval_type() == variable_type::integer) {
            if (rhs()->get<int32_t>() == 0) {
                throw_execute_error("division by zero");
            }
        } else if (rhs()->eval_type() == variable_type::floating) {
            if (rhs()->get<double>() == 0) {
                throw_execute_error("division by zero");
            }
        }

        using a = binary_arithmetic_node;

        const static std::array<std::array<void(binary_arithmetic_node::*)(), 2>, 2> table {{
             /*           int                  float          */
/* int    */{  &a::int_int<divides>,   &a::int_float<divides>  },
/* float  */{ &a::float_int<divides>, &a::float_float<divides> },
        }};

        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        assert(lhs_type != variable_type::error);
        assert(rhs_type != variable_type::error);

        auto lhs_index = static_cast<std::size_t>(lhs_type);
        auto rhs_index = static_cast<std::size_t>(rhs_type);
        
        (this->*table[lhs_index][rhs_index])();
    }
};

class modulus_node final : public binary_arithmetic_node {
 public:
    modulus_node(std::shared_ptr<expr_node> left , std::shared_ptr<expr_node> right)
        : binary_arithmetic_node(std::move(left), std::move(right)) {
        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        if (!is_both_int(lhs_type, rhs_type)) {
            throw_type_error("invalid operator % between {} and {}", lhs_type, rhs_type);
        }
    }

    void evaluate() override {
        lhs()->evaluate();
        rhs()->evaluate();

        int_int<modulus>();
    }
};

class logical_and_node : public binary_expr_node {
 public:
    logical_and_node(std::shared_ptr<expr_node> left, std::shared_ptr<expr_node> right)
        : binary_expr_node(std::move(left), std::move(right)) {
        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        if (!is_both_boolean(lhs_type, rhs_type)) {
            throw_type_error("invalid operator && between {} and {}", lhs_type, rhs_type);
        }
        set_value(bool{});
    }

    void evaluate() override {
        lhs()->evaluate();
        bool lhs_value = lhs()->get<bool>();
        if (!lhs_value) {
            set_value(false);
        } else {
            rhs()->evaluate();
            bool rhs_value = rhs()->get<bool>();
            set_value(lhs_value && rhs_value);
        }
    }
};

class logical_or_node : public binary_expr_node {
 public:
    logical_or_node(std::shared_ptr<expr_node> left, std::shared_ptr<expr_node> right)
        : binary_expr_node(std::move(left), std::move(right)) {
        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        if (!is_both_boolean(lhs_type, rhs_type)) {
            throw_type_error("invalid operator || between {} and {}", lhs_type, rhs_type);
        }
        set_value(bool{});
    }

    void evaluate() override {
        lhs()->evaluate();
        bool lhs_value = lhs()->get<bool>();
        if (lhs_value) {
            set_value(true);
        } else {
            rhs()->evaluate();
            bool rhs_value = rhs()->get<bool>();
            set_value(lhs_value || rhs_value);
        }
    }
};

class bit_and_node : public binary_expr_node {
 public:
    explicit bit_and_node(std::shared_ptr<expr_node> left, std::shared_ptr<expr_node> right)
        : binary_expr_node(std::move(left), std::move(right)) {
        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        if (!is_both_int(lhs_type, rhs_type)) {
            throw_type_error("invalid operator & between {} and {}", lhs_type, rhs_type);
        }
        set_value(int32_t{});
    }

    void evaluate() override {
        lhs()->evaluate();
        rhs()->evaluate();

        auto lhs_value = lhs()->get<int32_t>();
        auto rhs_value = rhs()->get<int32_t>();
        set_value(lhs_value & rhs_value);
    }
};

class bit_or_node : public binary_expr_node {
 public:
    explicit bit_or_node(std::shared_ptr<expr_node> left, std::shared_ptr<expr_node> right)
        : binary_expr_node(std::move(left), std::move(right)) {
        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        if (!is_both_int(lhs_type, rhs_type)) {
            throw_type_error("invalid operator | between {} and {}", lhs_type, rhs_type);
        }
        set_value(int32_t{});
    }

    void evaluate() override {
        lhs()->evaluate();
        rhs()->evaluate();

        auto lhs_value = lhs()->get<int32_t>();
        auto rhs_value = rhs()->get<int32_t>();
        set_value(lhs_value | rhs_value);
    }
};

class bit_xor_node : public binary_expr_node {
 public:
    explicit bit_xor_node(std::shared_ptr<expr_node> left, std::shared_ptr<expr_node> right)
        : binary_expr_node(std::move(left), std::move(right)) {
        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        if (!is_both_int(lhs_type, rhs_type)) {
            throw_type_error("invalid operator ^ between {} and {}", lhs_type, rhs_type);
        }
        set_value(int32_t{});
    }

    void evaluate() override {
        lhs()->evaluate();
        rhs()->evaluate();

        auto lhs_value = lhs()->get<int32_t>();
        auto rhs_value = rhs()->get<int32_t>();
        set_value(lhs_value ^ rhs_value);
    }
};

class shift_left_node : public binary_expr_node {
 public:
    explicit shift_left_node(std::shared_ptr<expr_node> left, std::shared_ptr<expr_node> right)
        : binary_expr_node(std::move(left), std::move(right)) {
        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        if (!is_both_int(lhs_type, rhs_type)) {
            throw_type_error("invalid operator << between {} and {}", lhs_type, rhs_type);
        }
    }

    void evaluate() override {
        lhs()->evaluate();
        rhs()->evaluate();

        auto lhs_value = lhs()->get<int32_t>();
        auto rhs_value = rhs()->get<int32_t>();

        if (rhs_value < 0) {
            throw_type_error("right operand of shift expression is negative: {}", rhs_value);
        }
        rhs_value %= std::numeric_limits<uint32_t>::digits;
        set_value(lhs_value << rhs_value);
    }
};

class shift_right_node : public binary_expr_node {
 public:
    explicit shift_right_node(std::shared_ptr<expr_node> left, std::shared_ptr<expr_node> right)
        : binary_expr_node(std::move(left), std::move(right)) {
        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        if (!is_both_int(lhs_type, rhs_type)) {
            throw_execute_error("invalid operator << between {} and {}", lhs_type, rhs_type);
        }
    }

    void evaluate() override {
        lhs()->evaluate();
        rhs()->evaluate();

        auto lhs_value = lhs()->get<int32_t>();
        auto rhs_value = rhs()->get<int32_t>();

        if (rhs_value < 0) {
            throw_execute_error("right operand of shift expression is negative: {}", rhs_value);
        }
        rhs_value %= std::numeric_limits<uint32_t>::digits;
        set_value(lhs_value >> rhs_value);
    }
};

class relation_node : public binary_expr_node {
 public:
    relation_node(std::shared_ptr<expr_node> lhs, std::shared_ptr<expr_node> rhs)
        : binary_expr_node(std::move(lhs), std::move(rhs)) {
        set_value(bool{});
    }

    template <typename Op>
    void int_int() {
        operation<int, int, Op>();
    }

    template <typename Op>
    void int_float() {
        operation<int, double, Op>();
    }

    template <typename Op>
    void float_int() {
        operation<double, int, Op>();
    }

    template <typename Op>
    void float_float() {
        operation<double, double, Op>();
    }

    template <typename Op>
    void bool_bool() {
        operation<bool, bool, Op>();
    }

    template <typename Op>
    void string_string() {
        operation<std::string, std::string, Op>();
    }

    template <typename Op>
    void char_char() {
        operation<char, char, Op>();
    }

};

class less_node : public relation_node {
 public:
    less_node(std::shared_ptr<expr_node> left, std::shared_ptr<expr_node> right)
        : relation_node(std::move(left), std::move(right)) {
        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();
        variable_type result_type = binary_expr_type(lhs_type, token_type::less, rhs_type);
        if (result_type == variable_type::error) {
            throw_type_error("invalid operator < between {} and {}", lhs_type, rhs_type);
        }
    }

    void evaluate() override {
        lhs()->evaluate();
        rhs()->evaluate();

        using a = relation_node;
        const static std::array<std::array<void(a::*)(), 5>, 5> table {{
             /*      int                  float            boolean         string               char          */
/* int    */{ &a::int_int<less>,    &a::int_float<less>,   nullptr,        nullptr,            nullptr        },
/* float  */{&a::float_int<less>,  &a::float_float<less>,  nullptr,        nullptr,            nullptr        },
/* bool   */{     nullptr,               nullptr,          nullptr,        nullptr,            nullptr        },
/* string */{     nullptr,               nullptr,          nullptr,  &a::string_string<less>,  nullptr        },
/* char   */{     nullptr,               nullptr,          nullptr,        nullptr,       &a::char_char<less> }
        }};

        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        auto lhs_index = static_cast<std::size_t>(lhs_type);
        auto rhs_index = static_cast<std::size_t>(rhs_type);
        
        (this->*table[lhs_index][rhs_index])();
    }
};

class less_equal_node : public relation_node {
 public:
    less_equal_node(std::shared_ptr<expr_node> left, std::shared_ptr<expr_node> right)
        : relation_node(std::move(left), std::move(right)) {
        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();
        variable_type result_type = binary_expr_type(lhs_type, token_type::less, rhs_type);
        if (result_type == variable_type::error) {
            throw_type_error("invalid operator <= between {} and {}", lhs_type, rhs_type);
        }
    }

    void evaluate() override {
        lhs()->evaluate();
        rhs()->evaluate();

        using a = relation_node;
        const static std::array<std::array<void(a::*)(), 5>, 5> table {{
             /*          int                        float              boolean             string                    char          */
/* int    */{ &a::int_int<less_equal>,    &a::int_float<less_equal>,   nullptr,           nullptr,                  nullptr        },
/* float  */{&a::float_int<less_equal>,  &a::float_float<less_equal>,  nullptr,           nullptr,                  nullptr        },
/* bool   */{          nullptr,                    nullptr,            nullptr,           nullptr,                  nullptr        },
/* string */{          nullptr,                    nullptr,            nullptr,  &a::string_string<less_equal>,     nullptr        },
/* char   */{          nullptr,                    nullptr,            nullptr,           nullptr,        &a::char_char<less_equal>}
        }};  

        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        auto lhs_index = static_cast<std::size_t>(lhs_type);
        auto rhs_index = static_cast<std::size_t>(rhs_type);
        
        (this->*table[lhs_index][rhs_index])();
    }
};

class greater_node : public relation_node {
 public:
    greater_node(std::shared_ptr<expr_node> left, std::shared_ptr<expr_node> right)
        : relation_node(std::move(left), std::move(right)) {
        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();
        variable_type result_type = binary_expr_type(lhs_type, token_type::less, rhs_type);
        if (result_type == variable_type::error) {
            throw_type_error("invalid operator > between {} and {}", lhs_type, rhs_type);
        }
    }

    void evaluate() override {
        lhs()->evaluate();
        rhs()->evaluate();

        using a = relation_node;
        const static std::array<std::array<void(a::*)(), 5>, 5> table {{
             /*       int                     float              boolean         string                      char          */
/* int    */{ &a::int_int<greater>,    &a::int_float<greater>,   nullptr,          nullptr,                 nullptr        },
/* float  */{&a::float_int<greater>,  &a::float_float<greater>,  nullptr,          nullptr,                 nullptr        },
/* bool   */{       nullptr,                 nullptr,            nullptr,          nullptr,                 nullptr        },
/* string */{       nullptr,                 nullptr,            nullptr,  &a::string_string<greater>,      nullptr        },
/* char   */{       nullptr,                 nullptr,            nullptr,          nullptr,         &a::char_char<greater> }
        }};

        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        auto lhs_index = static_cast<std::size_t>(lhs_type);
        auto rhs_index = static_cast<std::size_t>(rhs_type);
        
        (this->*table[lhs_index][rhs_index])();
    }
};

class greater_equal_node : public relation_node {
 public:
    greater_equal_node(std::shared_ptr<expr_node> left, std::shared_ptr<expr_node> right)
        : relation_node(std::move(left), std::move(right)) {
        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();
        variable_type result_type = binary_expr_type(lhs_type, token_type::less, rhs_type);
        if (result_type == variable_type::error) {
            throw_type_error("invalid operator >= between {} and {}", lhs_type, rhs_type);
        }
    }

    void evaluate() override {
        lhs()->evaluate();
        rhs()->evaluate();

        using a = relation_node;
        const static std::array<std::array<void(a::*)(), 5>, 5> table {{
             /*        int                              float                  boolean              string                       char           */
/* int    */{ &a::int_int<greater_equal>,     &a::int_float<greater_equal>,    nullptr,             nullptr,                    nullptr          },
/* float  */{&a::float_int<greater_equal>,   &a::float_float<greater_equal>,   nullptr,             nullptr,                    nullptr          },
/* bool   */{        nullptr,                          nullptr,                nullptr,             nullptr,                    nullptr          },
/* string */{        nullptr,                          nullptr,                nullptr,  &a::string_string<greater_equal>,      nullptr          },
/* char   */{        nullptr,                          nullptr,                nullptr,             nullptr,         &a::char_char<greater_equal>}
        }};

        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        auto lhs_index = static_cast<std::size_t>(lhs_type);
        auto rhs_index = static_cast<std::size_t>(rhs_type);
        
        (this->*table[lhs_index][rhs_index])();
    }
};

class equal_node : public relation_node {
 public:
    equal_node(std::shared_ptr<expr_node> left, std::shared_ptr<expr_node> right)
        : relation_node(std::move(left), std::move(right)) {
        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();
        variable_type result_type = binary_expr_type(lhs_type, token_type::less, rhs_type);
        if (result_type == variable_type::error) {
            throw_type_error("invalid operator == between {} and {}", lhs_type, rhs_type);
        }
    }

    void evaluate() override {
        lhs()->evaluate();
        rhs()->evaluate();

        using a = relation_node;
        const static std::array<std::array<void(a::*)(), 5>, 5> table {{
             /*      int                   float                  boolean              string                char          */
/* int    */{ &a::int_int<equal>,    &a::int_float<equal>,   &a::bool_bool<equal>,     nullptr,             nullptr        },
/* float  */{&a::float_int<equal>,  &a::float_float<equal>,       nullptr,             nullptr,             nullptr        },
/* bool   */{      nullptr,               nullptr,                nullptr,             nullptr,             nullptr        },
/* string */{      nullptr,               nullptr,                nullptr,  &a::string_string<equal>,       nullptr        },
/* char   */{      nullptr,               nullptr,                nullptr,             nullptr,       &a::char_char<equal> }
        }};

        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        auto lhs_index = static_cast<std::size_t>(lhs_type);
        auto rhs_index = static_cast<std::size_t>(rhs_type);
        
        (this->*table[lhs_index][rhs_index])();
    }
};

class not_equal_node : public relation_node {
 public:
    not_equal_node(std::shared_ptr<expr_node> left, std::shared_ptr<expr_node> right)
        : relation_node(std::move(left), std::move(right)) {
        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();
        variable_type result_type = binary_expr_type(lhs_type, token_type::less, rhs_type);
        if (result_type == variable_type::error) {
            throw_type_error("invalid operator != between {} and {}", lhs_type, rhs_type);
        }
    }

    void evaluate() override {
        lhs()->evaluate();
        rhs()->evaluate();

        using a = relation_node;
        const static std::array<std::array<void(a::*)(), 5>, 5> table {{
             /*        int                       float                      boolean                 string                    char          */
/* int    */{ &a::int_int<not_equal>,    &a::int_float<not_equal>,   &a::bool_bool<not_equal>,      nullptr,                 nullptr        },
/* float  */{&a::float_int<not_equal>,  &a::float_float<not_equal>,         nullptr,                nullptr,                 nullptr        },
/* bool   */{        nullptr,                   nullptr,                    nullptr,                nullptr,                 nullptr        },
/* string */{        nullptr,                   nullptr,                    nullptr,      &a::string_string<not_equal>,      nullptr        },
/* char   */{        nullptr,                   nullptr,                    nullptr,                nullptr,        &a::char_char<not_equal>}
        }};

        variable_type lhs_type = lhs()->eval_type();
        variable_type rhs_type = rhs()->eval_type();

        auto lhs_index = static_cast<std::size_t>(lhs_type);
        auto rhs_index = static_cast<std::size_t>(rhs_type);

        (this->*table[lhs_index][rhs_index])();
    }
};

class unary_node : public expr_node {
 public:
    explicit unary_node(std::shared_ptr<expr_node> expr)
        : expr_(std::move(expr)) {}
    
    virtual ~unary_node() = default;
    
    [[nodiscard]]
    std::shared_ptr<expr_node> expr() const noexcept {
        return expr_;
    }

 private:
    std::shared_ptr<expr_node> expr_;
};

class negative_node : public unary_node {
 public:
    explicit negative_node(std::shared_ptr<expr_node> exp)
        : unary_node(std::move(exp)) {
        if (expr()->eval_type() != variable_type::integer && expr()->eval_type() != variable_type::floating) {
            throw_type_error("invalid unary operator - for {}", expr()->eval_type());
        }

        variable_type type = expr()->eval_type();
        switch (type) {
            case variable_type::integer:
                set_value(int32_t{});
                break;
            case variable_type::floating:
                set_value(double{});
                break;
            default:
                std::unreachable();
        }
    }

    void evaluate() override {
        expr()->evaluate();
        if (expr()->eval_type() == variable_type::integer) {
            set_value(-expr()->get<int32_t>());
        } else {
            set_value(-expr()->get<double>());
        }
    }
};

class logical_not_node : public unary_node {
 public:
    explicit logical_not_node(std::shared_ptr<expr_node> exp)
        : unary_node(std::move(exp)) {
        if (expr()->eval_type() != variable_type::boolean) {
            throw_type_error("invalid unary operator ! for {}", expr()->eval_type());
        }
        set_value(bool{});
    }

    void evaluate() override {
        expr()->evaluate();
        bool value = expr()->get<bool>();
        if (value) {
            set_value(false);
        } else {
            set_value(true);
        }
    }
};

class bit_not_node : public unary_node {
 public:
    explicit bit_not_node(std::shared_ptr<expr_node> exp)
        : unary_node(std::move(exp)) {

        if (expr()->eval_type() != variable_type::integer) {
            throw_type_error("invalid operator ~ for {}", expr()->eval_type());
        }
        set_value(int32_t{});
    }

    void evaluate() override {
        expr()->evaluate();
        auto value = expr()->get<int32_t>();
        set_value(~value);
    }
};

class type_cast_node : public unary_node {
 public:
    type_cast_node(std::shared_ptr<expr_node> exp, variable_type target_type)
        : unary_node(std::move(exp)), target_type_(target_type) {}

    void evaluate() override {
        expr()->evaluate();
        variable_type original_type = expr()->eval_type();
        using a = type_cast_node;
        const static std::array<std::array<void(a::*)(), 5>, 5> table{{
            { nullptr, &a::type_cast<int32_t, double>, nullptr, nullptr, nullptr },
            { &a::type_cast<double, int32_t>, nullptr, nullptr, nullptr, nullptr },
            { nullptr, nullptr, nullptr, nullptr, nullptr },
            { nullptr, nullptr, nullptr, nullptr, nullptr },
            { &a::type_cast<char, int32_t>, nullptr, nullptr, nullptr, nullptr },
        }};

        auto l_index = static_cast<std::size_t>(original_type);
        auto r_index = static_cast<std::size_t>(target_type_);

        if (l_index == r_index) {
            set_value(expr()->value());
            return;
        }

        if (l_index >= 5 || r_index >= 5 || table[l_index][r_index] == nullptr) {
            throw_type_error("cannot cast {} to {}", original_type, target_type_);
        }

        (this->*table[l_index][r_index])();
    }

    template <typename OriginType, typename TargetType>
    void type_cast() {
        auto value = expr()->get<OriginType>();
        set_value(static_cast<TargetType>(value));
    }

 private:
    variable_type target_type_;
};

class new_node : public expr_node {
 public:
    new_node(variable_type type, std::vector<std::shared_ptr<expr_node>> sizes)
        : elem_type(type), size_per_dim(std::move(sizes)) {}

    void evaluate() override {
        set_value(build_array(elem_type, 0));
    }

 private:
    variable_type elem_type;
    std::vector<std::shared_ptr<expr_node>> size_per_dim;

    array build_array(variable_type elem_type, std::size_t dimension) {
        assert(!size_per_dim.empty());
        assert(dimension < size_per_dim.size());
        if (dimension == size_per_dim.size() - 1) {
            array arr;
            size_per_dim[dimension]->evaluate();
            auto size = size_per_dim[dimension]->get<int32_t>();
            for (int32_t i = 0; i < size; i++) {
                switch (elem_type) {
                    case variable_type::integer:
                        arr.push_back(int32_t{});
                        break;
                    case variable_type::floating:
                        arr.push_back(double{});
                        break;
                    case variable_type::boolean:
                        arr.push_back(bool{});
                        break;
                    case variable_type::string:
                        arr.push_back(std::string{});
                        break;
                    case variable_type::character:
                        arr.push_back(char{});
                        break;
                    default:
                        std::unreachable();
                }
            }
            return arr;
        }
        array arr;
        size_per_dim[dimension]->evaluate();
            auto size = size_per_dim[dimension]->get<int32_t>();
        for (int32_t i = 0; i < size; i++) {
            arr.push_back(build_array(elem_type, dimension + 1));
        }
        return arr;
    }
};

class int_node : public expr_node {
 public:
    explicit int_node(int32_t value) {
        set_value(value);
    }

    void evaluate() override {}
};

class float_node : public expr_node {
 public:
    explicit float_node(double value) {
        set_value(value);
    }

    void evaluate() override {}
};

class boolean_node : public expr_node {
 public:
    explicit boolean_node(bool value) {
        set_value(value);
    }

    void evaluate() override {}
};

class string_node : public expr_node {
 public:
    explicit string_node(std::string str) {
        set_value(std::move(str));
    }

    void evaluate() override {}
};

class char_node : public expr_node {
 public:
    explicit char_node(char value) {
        set_value(value);
    }

    void evaluate() override {}
};

class array_node : public expr_node {
 public:
    explicit array_node(value_t value, variable_type value_type)
        : value_type_(value_type) {
        set_value(std::move(value));
    }

    void evaluate() override {}

    [[nodiscard]]
    std::size_t size() const {
        assert(eval_type() == variable_type::array);
        return get<array>().size();
    }

    [[nodiscard]]
    bool empty() const {
        assert(eval_type() == variable_type::array);
        return get<array>().empty();
    }

    [[nodiscard]]
    const value_t &at(std::size_t index) const {
        return get<array>().operator[](index);
    }

    void set(std::size_t index, value_t value) {
        array &data = get<array>();
        data[index] = std::move(value);
    }

    [[nodiscard]]
    variable_type value_type() const noexcept {
        return value_type_;
    }

 private:
    variable_type value_type_;
};

enum class execute_state {
    normal, broken, continued, returned
};

class statement_node : public ast_node {
 public:
    virtual ~statement_node() {}

    virtual std::pair<execute_state, std::optional<value_t>> execute() = 0;
};

class expr_stat_node : public statement_node, public expr_node {
 public:
    expr_stat_node(std::shared_ptr<expr_node> expr)
        : expr_(std::move(expr)) {}
    
    std::pair<execute_state, std::optional<value_t>> execute() override {
        expr_->evaluate();
        return {execute_state::normal, std::nullopt};
    }

    void evaluate() override {
        expr_->evaluate();
    }

 private:
    std::shared_ptr<expr_node> expr_;
};

class for_node : public statement_node {
 public:
    for_node(std::shared_ptr<expr_stat_node> init,
             std::shared_ptr<expr_stat_node> condition,
             std::shared_ptr<expr_node>      update,
             std::shared_ptr<statement_node> statements)
        : init_(std::move(init)), condition_(std::move(condition)),
          update_(std::move(update)), statements_(std::move(statements)) {}

    std::pair<execute_state, std::optional<value_t>> execute() override {
        assert(condition_->eval_type() == variable_type::boolean);
        init_->execute();
        condition_->evaluate();
        while (condition_->get<bool>()) {
            auto [state, returned] = statements_->execute();
            if (state == execute_state::continued) {
                update_->evaluate();
                condition_->evaluate();
                continue;
            }
            if (state == execute_state::broken) {
                break;
            }
            if (state == execute_state::returned) {
                return {execute_state::returned, returned};
            }
            update_->evaluate();
            condition_->evaluate();
        }
        return {execute_state::normal, std::nullopt};
    }

 private:
    std::shared_ptr<expr_stat_node> init_;
    std::shared_ptr<expr_stat_node> condition_;
    std::shared_ptr<expr_node>      update_;
    std::shared_ptr<statement_node> statements_;
};

class while_node : public statement_node {
 public:
    while_node(std::shared_ptr<expr_node> condition, std::shared_ptr<statement_node> body)
        : condition_(std::move(condition)), body_(std::move(body)) {}

    std::pair<execute_state, std::optional<value_t>> execute() override {
        assert(condition_->eval_type() == variable_type::boolean);
        condition_->evaluate();
        while (condition_->get<bool>()) {
            auto [state, returned] = body_->execute();
            if (state == execute_state::continued) {
                condition_->evaluate();
                continue;
            }
            if (state == execute_state::broken) {
                break;
            }
            if (state == execute_state::returned) {
                return {execute_state::returned, returned};
            }
        }
        return {execute_state::normal, std::nullopt};
    }

 private:
    std::shared_ptr<expr_node> condition_;
    std::shared_ptr<statement_node> body_;
};

class continue_node : public statement_node {
 public:
    std::pair<execute_state, std::optional<value_t>> execute() override {
        return {execute_state::continued, std::nullopt};
    }
};

class break_node : public statement_node {
 public:
    std::pair<execute_state, std::optional<value_t>> execute() override {
        return {execute_state::broken, std::nullopt};
    }
};

class return_node : public statement_node {
 public:
    // pass `nullptr` to return void
    return_node(std::shared_ptr<expr_node> expr)
        : expr_(std::move(expr)) {}

    std::pair<execute_state, std::optional<value_t>> execute() override {
        if (expr_ == nullptr) {
            return {execute_state::returned, std::nullopt};
        }
        expr_->evaluate();
        return {execute_state::returned, expr_->value()};
    }

 private:
    std::shared_ptr<expr_node> expr_;
};

}   // namespace detail

}   // namespace script

}   // namespace neroll

#endif