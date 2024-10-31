#ifndef NEROLL_SCRIPT_DETAIL_OPERATOR_H
#define NEROLL_SCRIPT_DETAIL_OPERATOR_H

namespace neroll {

namespace script {

namespace detail {

struct plus {
    template <typename T, typename U>
    constexpr auto operator()(const T &lhs, const U &rhs) {
        return lhs + rhs;
    }
};

struct minus {
    template <typename T, typename U>
    constexpr auto operator()(const T &lhs, const U &rhs) {
        return lhs - rhs;
    }
};

struct multiplies {
    template <typename T, typename U>
    constexpr auto operator()(const T &lhs, const U &rhs) {
        return lhs * rhs;
    }
};


struct divides {
    template <typename T, typename U>
    constexpr auto operator()(const T &lhs, const U &rhs) {
        return lhs / rhs;
    }
};

struct modulus {
    template <typename T, typename U>
    constexpr auto operator()(const T &lhs, const U &rhs) {
        return lhs % rhs;
    }
};

struct bit_and {
    template <typename T, typename U>
    constexpr auto operator()(const T &lhs, const U &rhs) {
        return lhs & rhs;
    }
};

struct bit_or {
    template <typename T, typename U>
    constexpr auto operator()(const T &lhs, const U &rhs) {
        return lhs | rhs;
    }
};

struct bit_xor {
    template <typename T, typename U>
    constexpr auto operator()(const T &lhs, const U &rhs) {
        return lhs ^ rhs;
    }
};

struct less {
    template <typename T, typename U>
    constexpr auto operator()(const T &lhs, const U &rhs) {
        return lhs < rhs;
    }
};

struct less_equal {
    template <typename T, typename U>
    constexpr auto operator()(const T &lhs, const U &rhs) {
        return lhs <= rhs;
    }
};

struct greater {
    template <typename T, typename U>
    constexpr auto operator()(const T &lhs, const U &rhs) {
        return lhs > rhs;
    }
};

struct greater_equal {
    template <typename T, typename U>
    constexpr auto operator()(const T &lhs, const U &rhs) {
        return lhs >= rhs;
    }
};

struct equal {
    template <typename T, typename U>
    constexpr auto operator()(const T &lhs, const U &rhs) {
        return lhs == rhs;
    }
};

struct not_equal {
    template <typename T, typename U>
    constexpr auto operator()(const T &lhs, const U &rhs) {
        return lhs != rhs;
    }
};

}   // namespace detail

}   // namespace script

}   // namespace neroll

#endif