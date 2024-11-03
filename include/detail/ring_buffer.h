#ifndef NEROLL_SCRIPT_DETAIL_RING_BUFFER_H
#define NEROLL_SCRIPT_DETAIL_RING_BUFFER_H

#include <cstddef>      // size_t
#include <array>        // array
#include <cassert>      // assert
#include <type_traits>  // is_default_constructible_v

namespace neroll::script::detail {

template <typename T, std::size_t N>
    requires std::is_default_constructible_v<T>
class ring_buffer {
 public:
    ring_buffer() = default;

    [[nodiscard]]
    std::size_t capacity() const noexcept {
        return data.size();
    }

    void put(const T &value) {
        data[cur_pos] = value;
        cur_pos = (cur_pos + 1) % data.size();
    }

    const T &get_next(std::size_t distance) const {
        assert(distance < data.size());
        return data[(cur_pos + distance) % data.size()];
    }

 private:
    std::size_t cur_pos{};
    std::array<T, N> data;
};

}   // namespace neroll::script::detail

#endif